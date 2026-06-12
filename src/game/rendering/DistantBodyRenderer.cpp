#include "rendering/DistantBodyRenderer.h"
#include "world/PlanetVisualGenerator.h"
#include "rendering/PlanetMeshGenerator.h"
#include "rendering/PlanetCloudGenerator.h"
#include "rendering/PlanetMaterialGenerator.h"

#include <rlgl.h>

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <string>

namespace SpaceSim
{
    namespace
    {
        static float GetCloudShellScale(const GlobalObject& object)
        {
            if (!object.hasPlanetData)
            {
                return 1.03f;
            }

            switch (object.planetData.planetClass)
            {
            case PlanetClass::OceanWorld:
                return 1.025f;  // needs more clearance because terrain + cloud coverage
            case PlanetClass::DesertWorld:
                return 1.035f;
            case PlanetClass::Rocky:
                return 1.03f;
            case PlanetClass::CarbonWorld:
                return 1.03f;
            case PlanetClass::LavaWorld:
                return 1.03f;
            default:
                return 1.04f;
            }
        }

        static float GetAtmosphereShellScale(const GlobalObject& object)
        {
            if (object.hasPlanetData &&
                object.planetData.planetClass == PlanetClass::OceanWorld)
            {
                return 1.038f;
            }

            return GetCloudShellScale(object) + 0.012f;
        }
        static bool ShouldDrawAtmosphere(const GlobalObject& object, const PlanetVisual& visual)
        {
            if (!object.hasPlanetData)
            {
                return false;
            }

            return object.planetData.atmosphere != AtmosphereType::None &&
                visual.atmosphereStrength > 0.0f &&
                visual.atmosphereColor.a > 0;
        }
        constexpr float SkyBodyRenderDistance = 900.0f;

        struct DominantLight
        {
            Vector3 direction{};
            Vector3 color{ 1.0f, 0.97f, 0.90f };
            float intensity = 1.0f;
        };

        static Vector3 ColorToVector3(Color color)
        {
            return Vector3{
                color.r / 255.0f,
                color.g / 255.0f,
                color.b / 255.0f
            };
        }

        static DominantLight ComputeDominantLightForObject(
            const GameWorld& world,
            Vector3 objectRenderPosition
        )
        {
            const GlobalObject* sunObject = nullptr;

            for (const GlobalObject& candidate : world.starSystem.objects)
            {
                if (candidate.type == GlobalObjectType::Sun)
                {
                    sunObject = &candidate;
                    break;
                }
            }

            if (sunObject == nullptr)
            {
                return DominantLight{
                    Vector3Normalize(Vector3{ -0.55f, 0.22f, -0.80f }),
                    Vector3{ 1.0f, 0.97f, 0.90f },
                    1.0f
                };
            }

            // Important:
            // Distant bodies are not rendered at their real global positions.
            // They are projected onto the camera-centered sky shell. Therefore the
            // lighting direction should be computed in the same render-space the player sees,
            // not directly from raw global object coordinates.
            DVec3 relativeSun = sunObject->position - world.globalPlayerPosition;
            relativeSun = Normalize(relativeSun);

            Vector3 sunDirectionFromCamera{
                static_cast<float>(relativeSun.x),
                static_cast<float>(relativeSun.y),
                static_cast<float>(relativeSun.z)
            };
            Vector3 sunRenderPosition = Vector3Scale(
                sunDirectionFromCamera,
                SkyBodyRenderDistance
            );

            Vector3 lightDirection = Vector3Subtract(
                sunRenderPosition,
                objectRenderPosition
            );

            if (Vector3Length(lightDirection) < 0.001f)
            {
                lightDirection = sunDirectionFromCamera;
            }

            Vector3 lightColor = ColorToVector3(sunObject->color);

            // Keep the prototype light stable and readable.
            // Later this can be driven by star class/luminosity data.
            return DominantLight{
                Vector3Normalize(lightDirection),
                lightColor,
                1.0f
            };
        }

        static Model& GetAtmosphereShellModel()
        {
            static bool loaded = false;
            static Model model{};

            if (loaded)
            {
                return model;
            }

            Mesh mesh = GenMeshSphere(1.0f, 64, 32);
            model = LoadModelFromMesh(mesh);

            loaded = true;
            return model;
        }
        static Shader& GetAtmosphereShader()
        {
            static bool loaded = false;
            static Shader shader{};

            if (loaded)
            {
                return shader;
            }

            const char* vertexShader = R"(
        #version 330

        in vec3 vertexPosition;
        in vec3 vertexNormal;

        uniform mat4 mvp;
        uniform mat4 matModel;

        out vec3 fragWorldPos;
        out vec3 fragNormal;

        void main()
        {
            vec4 worldPos = matModel * vec4(vertexPosition, 1.0);
            fragWorldPos = worldPos.xyz;
            fragNormal = normalize(mat3(matModel) * vertexNormal);

            gl_Position = mvp * vec4(vertexPosition, 1.0);
        }
        )" ;

            const char* fragmentShader = R"(
        #version 330

        in vec3 fragWorldPos;
        in vec3 fragNormal;

        out vec4 finalColor;

        uniform vec3 planetCenter;
        uniform vec3 atmosphereColor;
        uniform vec3 sunDir;
        uniform float strength;

        void main()
        {
            vec3 normal = normalize(fragNormal);

            // Camera is effectively at local origin for this sky/distant-body render.
            vec3 viewDir = normalize(-fragWorldPos);

            float viewDot = clamp(dot(normal, viewDir), 0.0, 1.0);

            // Fresnel rim: visible mainly at the edge, not across the whole sphere.
            float rim = pow(1.0 - viewDot, 3.2);

            float rawSun = max(dot(normal, normalize(sunDir)), 0.0);
            float day = pow(rawSun, 1.35);

            // Lit-side blue rim only. Keep the atmosphere from washing the whole day side.
            float alpha = rim * mix(0.0, 0.34, day) * strength;

            // Cut extremely low alpha so the full shell does not become a visible bubble.
            if (alpha < 0.01)
            {
                discard;
            }

            finalColor = vec4(atmosphereColor, alpha);
        }
        )";

            shader = LoadShaderFromMemory(vertexShader, fragmentShader);
            loaded = true;

            return shader;
        }

        static Shader& GetPlanetSurfaceShader()
        {
            static bool loaded = false;
            static Shader shader{};

            if (loaded)
            {
                return shader;
            }

            const char* vertexShader = R"(
        #version 330

        in vec3 vertexPosition;
        in vec3 vertexNormal;
        in vec4 vertexColor;

        uniform mat4 mvp;
        uniform mat4 matModel;

        out vec3 fragWorldPos;
        out vec3 fragWorldNormal;
        out vec3 fragObjectNormal;

        void main()
        {
            vec4 worldPos = matModel * vec4(vertexPosition, 1.0);

            fragWorldPos = worldPos.xyz;
            fragWorldNormal = normalize(mat3(matModel) * vertexNormal);
            fragObjectNormal = normalize(vertexNormal);

            gl_Position = mvp * vec4(vertexPosition, 1.0);
        }
    )";

            const char* fragmentShader = R"(
        #version 330

        in vec3 fragWorldPos;
        in vec3 fragWorldNormal;
        in vec3 fragObjectNormal;

        out vec4 finalColor;

        uniform sampler2D albedoTex;
        uniform sampler2D normalTex;
        uniform sampler2D materialTex;
        uniform vec3 sunDir;
        uniform vec3 lightColor;
        uniform float lightIntensity;
        uniform mat4 matModel;
        uniform int debugPlanetMaterial;
        uniform int debugOceanSpecular;
        uniform int debugOceanSunLobe;

        const float PI = 3.14159265358979323846;

        vec2 dirToUv(vec3 dir)
        {
            dir = normalize(dir);

            float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
            float v = acos(clamp(dir.y, -1.0, 1.0)) / PI;

            return vec2(u, v);
        }

        float DistributionGGX(float NdotH, float roughness)
        {
            float a = roughness * roughness;
            float a2 = a * a;
            float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
            return a2 / max(PI * denom * denom, 0.0001);
        }

        float GeometrySchlickGGX(float NdotX, float roughness)
        {
            float r = roughness + 1.0;
            float k = (r * r) / 8.0;
            return NdotX / max(NdotX * (1.0 - k) + k, 0.0001);
        }

        float GeometrySmith(float NdotV, float NdotL, float roughness)
        {
            float ggxV = GeometrySchlickGGX(NdotV, roughness);
            float ggxL = GeometrySchlickGGX(NdotL, roughness);
            return ggxV * ggxL;
        }

        vec3 FresnelSchlick(float cosTheta, vec3 F0)
        {
            return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
        }

        float Hash31(vec3 p)
        {
            p = fract(p * 0.1031);
            p += dot(p, p.yzx + 33.33);
            return fract((p.x + p.y) * p.z);
        }

        float ValueNoise(vec3 p)
        {
            vec3 i = floor(p);
            vec3 f = fract(p);
            f = f * f * (3.0 - 2.0 * f);

            float n000 = Hash31(i + vec3(0.0, 0.0, 0.0));
            float n100 = Hash31(i + vec3(1.0, 0.0, 0.0));
            float n010 = Hash31(i + vec3(0.0, 1.0, 0.0));
            float n110 = Hash31(i + vec3(1.0, 1.0, 0.0));
            float n001 = Hash31(i + vec3(0.0, 0.0, 1.0));
            float n101 = Hash31(i + vec3(1.0, 0.0, 1.0));
            float n011 = Hash31(i + vec3(0.0, 1.0, 1.0));
            float n111 = Hash31(i + vec3(1.0, 1.0, 1.0));

            float nx00 = mix(n000, n100, f.x);
            float nx10 = mix(n010, n110, f.x);
            float nx01 = mix(n001, n101, f.x);
            float nx11 = mix(n011, n111, f.x);

            float nxy0 = mix(nx00, nx10, f.y);
            float nxy1 = mix(nx01, nx11, f.y);

            return mix(nxy0, nxy1, f.z);
        }

        float Fbm(vec3 p)
        {
            float total = 0.0;
            float amp = 0.5;
            for (int i = 0; i < 5; ++i)
            {
                total += ValueNoise(p) * amp;
                p *= 2.03;
                amp *= 0.5;
            }
            return total;
        }

        void main()
        {
            vec3 sphereNormal = normalize(fragWorldNormal);
            vec3 objectNormal = normalize(fragObjectNormal);

            // In the sky-body pass the camera is at the origin.
            vec3 V = normalize(-fragWorldPos);
            vec3 L = normalize(sunDir);
            vec3 H = normalize(V + L);

            vec2 uv = dirToUv(objectNormal);

            vec4 albedoSample = texture(albedoTex, uv);
            vec4 materialSample = texture(materialTex, uv);

            vec3 baseColorSrgb = max(albedoSample.rgb, vec3(0.020));
            vec3 albedo = pow(baseColorSrgb, vec3(2.2));

            float landMask = albedoSample.a;
            float waterMask = materialSample.r;
            float coastMask = materialSample.g;
            float landRoughness = materialSample.b;
            float terrainRelief = materialSample.a;

            if (debugPlanetMaterial == 1)
            {
                finalColor = vec4(waterMask, landMask, coastMask, 1.0);
                return;
            }

            vec3 normalSampleObject = texture(normalTex, uv).rgb * 2.0 - 1.0;
            vec3 normalSampleWorld = normalize(mat3(matModel) * normalSampleObject);

            float landNormalStrength = landMask * mix(0.40, 1.15, terrainRelief);
            vec3 landNormal = normalize(mix(sphereNormal, normalSampleWorld, landNormalStrength));

            // Procedural water micro-normal. This only shapes the glint; the broad day/night
            // lighting still uses the sphere normal so the planet remains visually stable.
            vec3 objectUp = abs(objectNormal.y) < 0.95 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
            vec3 oceanTangent = normalize(cross(objectUp, objectNormal));
            vec3 oceanBitangent = normalize(cross(objectNormal, oceanTangent));

            float oceanMacro = Fbm(objectNormal * 7.5 + vec3(4.0, 9.0, 2.0));
            float oceanFine = Fbm(objectNormal * 42.0 + vec3(12.0, 1.0, 8.0));
            float waveA = Fbm(objectNormal * 18.0 + vec3(13.1, 7.7, 3.4));
            float waveB = Fbm(objectNormal * 31.0 + vec3(31.4, 2.1, 17.8));
            float waveC = Fbm(objectNormal * 78.0 + vec3(5.3, 19.7, 11.2));

            vec3 oceanNormalObject = normalize(
                objectNormal +
                oceanTangent * ((waveA - 0.5) * 0.26 + (waveC - 0.5) * 0.06) +
                oceanBitangent * ((waveB - 0.5) * 0.24 + (oceanFine - 0.5) * 0.05)
            );
            vec3 oceanNormal = normalize(mat3(matModel) * oceanNormalObject);

            float sphereNdotLRaw = dot(sphereNormal, L);
            float sphereNdotL = max(sphereNdotLRaw, 0.0);
            float dayGate = smoothstep(-0.025, 0.080, sphereNdotLRaw);

            float landNdotL = max(dot(landNormal, L), 0.0);
            float waterNdotL = max(dot(oceanNormal, L), 0.0);

            if (debugOceanSpecular == 1)
            {
                finalColor = vec4(vec3(pow(sphereNdotL, 1.45)), 1.0);
                return;
            }

            // -------------------------------------------------------------------------
            // Simple Phong material split
            // -------------------------------------------------------------------------
            // This intentionally removes the previous PBR-ish stack so the behavior is obvious:
            // final = ambient + diffuse + specular.  Land gets almost no specular.  Water gets a
            // strong white/blue Phong sun reflection.

            vec3 Nland = normalize(landNormal);
            vec3 Nwater = normalize(mix(sphereNormal, oceanNormal, 0.55));

            float landDiffuseRaw = max(dot(Nland, L), 0.0);
            float waterDiffuseRaw = max(dot(Nwater, L), 0.0);

            // Use the sphere normal as the broad day/night gate.  This prevents noisy normals from
            // breaking the planet-wide terminator while still letting material normals affect detail.
            float day = smoothstep(-0.030, 0.075, sphereNdotLRaw);

            // ----- Land: matte Phong -----
            vec3 landAlbedo = albedo;
            float landAmbient = 0.018;
            float landDiffuse = pow(max(mix(sphereNdotL, landDiffuseRaw, 0.30), 0.0), 1.20);
            vec3 landColor = landAlbedo * (landAmbient + landDiffuse * 1.05 * lightIntensity) * lightColor;

            float slopeAmount = 1.0 - clamp(dot(Nland, sphereNormal), 0.0, 1.0);
            landColor *= mix(1.0, 0.58, slopeAmount * terrainRelief * 2.8);

            // Tiny land specular: just enough for rock variation, not enough to look wet.
            vec3 Rland = reflect(-L, Nland);
            float landSpec = pow(max(dot(Rland, V), 0.0), 36.0);
            landSpec *= landMask * day * (1.0 - landRoughness) * 0.010;
            landColor += vec3(0.12, 0.10, 0.07) * landSpec;

            // ----- Water: dark diffuse + strong Phong sun glint -----
            vec3 deepOcean = vec3(0.003, 0.014, 0.038);
            vec3 shallowOcean = vec3(0.012, 0.045, 0.085);
            vec3 oceanAlbedo = mix(deepOcean, shallowOcean, clamp(coastMask * 0.65 + oceanMacro * 0.12, 0.0, 1.0));

            float waterAmbient = 0.004;
            float waterDiffuse = pow(waterDiffuseRaw, 1.45);
            vec3 oceanColor = oceanAlbedo * (waterAmbient + waterDiffuse * 0.11 * lightIntensity) * lightColor;

            // Classic Phong specular.  This is deliberately broad and strong for debugging/readability.
            // If this does not visibly separate ocean from land, the issue is not PBR vs non-PBR; it is
            // the water mask, camera/sun geometry, or output path.
            vec3 Rwater = reflect(-L, Nwater);
            float phongAlign = max(dot(Rwater, V), 0.0);

            float broadWaterSpec = pow(phongAlign, 8.0);     // large orbit-scale reflection patch
            float tightWaterSpec = pow(phongAlign, 55.0);    // bright sun-core glint
            float glitter = 0.72 + oceanMacro * 0.28 + oceanFine * 0.25;

            vec3 waterSpecular = lightColor * waterMask * day * glitter *
                (broadWaterSpec * 1.25 + tightWaterSpec * 4.50);

            // Small Fresnel edge sheen on water only; lower than before so it does not look like a
            // mysterious side light around the whole planet.
            float fresnel = pow(1.0 - max(dot(Nwater, V), 0.0), 5.0);
            vec3 waterFresnel = vec3(0.015, 0.060, 0.13) * fresnel * waterMask * day * 0.22;

            vec3 litColor = mix(oceanColor, landColor, landMask);

            // Apply day/night only to diffuse base.  Specular is added afterward, like Phong.
            float diffuseShadow = mix(0.015, 1.0, pow(sphereNdotL, 1.08)) * day;
            litColor *= diffuseShadow;

            litColor += waterSpecular;
            litColor += waterFresnel;

            if (debugOceanSunLobe == 1)
            {
                // F8 debug:
                // red   = water mask on the lit side
                // green = classic Phong water specular
                // blue  = water Fresnel
                finalColor = vec4(
                    clamp(waterMask * sphereNdotL, 0.0, 1.0),
                    clamp((broadWaterSpec * 0.70 + tightWaterSpec * 2.50) * waterMask * day, 0.0, 1.0),
                    clamp(fresnel * waterMask * day, 0.0, 1.0),
                    1.0
                );
                return;
            }

            vec3 outputColor = pow(clamp(litColor, 0.0, 1.0), vec3(1.0 / 2.2));
            finalColor = vec4(outputColor, 1.0);
        }
        )";

        shader = LoadShaderFromMemory(vertexShader, fragmentShader);

        if (shader.id == 0 || shader.locs == nullptr)
        {
            loaded = true;
            return shader;
        }

        shader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(shader, "albedoTex");
        shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "normalTex");
        shader.locs[SHADER_LOC_MAP_SPECULAR] = GetShaderLocation(shader, "materialTex");

        loaded = true;

            return shader;
        }
        static PlanetMaterialMaps& GetPlanetMaterialMaps(const GlobalObject& object)
        {
            static std::unordered_map<std::string, PlanetMaterialMaps> materialCache;

            auto found = materialCache.find(object.id);
            if (found != materialCache.end())
            {
                return found->second;
            }

            const bool highResolution = object.hasPlanetData &&
                object.planetData.planetClass == PlanetClass::OceanWorld;

            PlanetMaterialMaps maps = GeneratePlanetMaterialMaps(
                object,
                highResolution ? 1024 : 512,
                highResolution ? 512 : 256
            );

            auto inserted = materialCache.emplace(object.id, maps);
            return inserted.first->second;
        }
        static void ApplyPlanetSurfaceShader(
            const GlobalObject& object,
            Model& model,
            const DominantLight& light
        )
        {
            if (!object.hasPlanetData)
            {
                return;
            }

            Shader& planetShader = GetPlanetSurfaceShader();

            if (planetShader.id == 0 || planetShader.locs == nullptr)
            {
                return;
            }

            PlanetMaterialMaps& maps = GetPlanetMaterialMaps(object);

            model.materials[0].shader = planetShader;

            // All planets now use the same shader contract:
            // diffuse = raw albedo, normal = object-space normal map,
            // specular = packed material masks.
            model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = maps.albedo;
            model.materials[0].maps[MATERIAL_MAP_NORMAL].texture = maps.normal;
            model.materials[0].maps[MATERIAL_MAP_SPECULAR].texture = maps.material;

            SetShaderValue(
                planetShader,
                GetShaderLocation(planetShader, "sunDir"),
                &light.direction,
                SHADER_UNIFORM_VEC3
            );

            SetShaderValue(
                planetShader,
                GetShaderLocation(planetShader, "lightColor"),
                &light.color,
                SHADER_UNIFORM_VEC3
            );

            SetShaderValue(
                planetShader,
                GetShaderLocation(planetShader, "lightIntensity"),
                &light.intensity,
                SHADER_UNIFORM_FLOAT
            );

            int debugPlanetMaterial = IsKeyDown(KEY_F6) ? 1 : 0;
            SetShaderValue(
                planetShader,
                GetShaderLocation(planetShader, "debugPlanetMaterial"),
                &debugPlanetMaterial,
                SHADER_UNIFORM_INT
            );

            int debugOceanSpecular = IsKeyDown(KEY_F7) ? 1 : 0;
            SetShaderValue(
                planetShader,
                GetShaderLocation(planetShader, "debugOceanSpecular"),
                &debugOceanSpecular,
                SHADER_UNIFORM_INT
            );

            int debugOceanSunLobe = IsKeyDown(KEY_F8) ? 1 : 0;
            SetShaderValue(
                planetShader,
                GetShaderLocation(planetShader, "debugOceanSunLobe"),
                &debugOceanSunLobe,
                SHADER_UNIFORM_INT
            );
        }
        static Shader& GetCloudShader()
        {
            static bool loaded = false;
            static Shader shader{};

            if (loaded)
            {
                return shader;
            }

            const char* vertexShader = R"(
        #version 330

        in vec3 vertexPosition;
        in vec3 vertexNormal;
        in vec4 vertexColor;

        uniform mat4 mvp;
        uniform mat4 matModel;

        out vec3 fragDir;
        out vec4 fragVertexColor;

        void main()
        {
            fragDir = normalize(mat3(matModel) * vertexNormal);
            fragVertexColor = vertexColor;

            gl_Position = mvp * vec4(vertexPosition, 1.0);
        }
        )";

            const char* fragmentShader = R"(
        #version 330

        in vec3 fragDir;
        in vec4 fragVertexColor;

        out vec4 finalColor;

        uniform float cloudSeed;
        uniform vec3 sunDir;

        float hash(vec3 p)
        {
            p = fract(p * 0.3183099 + vec3(0.11, 0.17, 0.13));
            p *= 17.0;
            return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
        }

        float noise(vec3 p)
        {
            vec3 i = floor(p);
            vec3 f = fract(p);

            f = f * f * (3.0 - 2.0 * f);

            float n000 = hash(i + vec3(0.0, 0.0, 0.0));
            float n100 = hash(i + vec3(1.0, 0.0, 0.0));
            float n010 = hash(i + vec3(0.0, 1.0, 0.0));
            float n110 = hash(i + vec3(1.0, 1.0, 0.0));

            float n001 = hash(i + vec3(0.0, 0.0, 1.0));
            float n101 = hash(i + vec3(1.0, 0.0, 1.0));
            float n011 = hash(i + vec3(0.0, 1.0, 1.0));
            float n111 = hash(i + vec3(1.0, 1.0, 1.0));

            float nx00 = mix(n000, n100, f.x);
            float nx10 = mix(n010, n110, f.x);
            float nx01 = mix(n001, n101, f.x);
            float nx11 = mix(n011, n111, f.x);

            float nxy0 = mix(nx00, nx10, f.y);
            float nxy1 = mix(nx01, nx11, f.y);

            return mix(nxy0, nxy1, f.z);
        }

        float fbm(vec3 p)
        {
            float total = 0.0;
            float amp = 0.5;

            for (int i = 0; i < 5; ++i)
            {
                total += noise(p) * amp;
                p *= 2.05;
                amp *= 0.5;
            }

            return total;
        }

        void main()
        {
            vec3 dir = normalize(fragDir);

            // Slightly bend the sampling coordinates by latitude so the clouds
            // look more like stretched atmospheric systems instead of random blobs.
            vec3 p = dir;
            p.x += sin(dir.y * 7.0 + cloudSeed) * 0.18;
            p.z += cos(dir.y * 6.0 + cloudSeed * 0.7) * 0.18;

            float broad = fbm(p * 2.35 + cloudSeed);
            float medium = fbm(p * 7.50 + cloudSeed * 1.71);
            float fine = fbm(p * 22.0 + cloudSeed * 2.37);

            float cloud = broad * 0.64 + medium * 0.28 + fine * 0.08;

            // Stricter coverage: the old shell covered too much of the planet and flattened lighting.
            float mask = smoothstep(0.62, 0.82, cloud);

            // Edge breakup should thin clouds, not erase them completely.
            float breakup = smoothstep(0.20, 0.68, medium + fine * 0.35);
            mask *= mix(0.55, 1.0, breakup);

            // Use a real directional day curve. The old smoothstep kept most of the day side
            // uniformly bright and made the surface lighting underneath look flat.
            float rawSun = max(dot(dir, normalize(sunDir)), 0.0);
            float day = pow(rawSun, 1.35);

            // Clouds should be visible details, not a translucent white blanket.
            float alpha = mask * mix(0.0, 0.095, day);

            if (alpha < 0.01)
            {
                discard;
            }

            vec3 nightColor = vec3(0.00, 0.00, 0.00);
            vec3 dayColor = vec3(0.82, 0.86, 0.90);
            
            vec3 color = mix(nightColor, dayColor, day);

            finalColor = vec4(color, alpha);
        }
        )";

            shader = LoadShaderFromMemory(vertexShader, fragmentShader);
            loaded = true;

            return shader;
        }
        static void DrawAtmosphereShell(
            const GlobalObject& object,
            const PlanetVisual& visual,
            Vector3 position,
            float radius,
            Vector3 sunDirection
        )
        {
            if (!ShouldDrawAtmosphere(object, visual))
            {
                return;
            }

            Model& atmosphereModel = GetAtmosphereShellModel();
            Shader& atmosphereShader = GetAtmosphereShader();

            atmosphereModel.materials[0].shader = atmosphereShader;

            Vector3 atmosphereColor{
                visual.atmosphereColor.r / 255.0f,
                visual.atmosphereColor.g / 255.0f,
                visual.atmosphereColor.b / 255.0f
            };

            if (object.hasPlanetData &&
                object.planetData.planetClass == PlanetClass::OceanWorld)
            {
                atmosphereColor = Vector3{ 0.25f, 0.55f, 1.0f };
            }

            float strength = Clamp(visual.atmosphereStrength * 0.85f, 0.10f, 0.85f);

            SetShaderValue(
                atmosphereShader,
                GetShaderLocation(atmosphereShader, "planetCenter"),
                &position,
                SHADER_UNIFORM_VEC3
            );

            SetShaderValue(
                atmosphereShader,
                GetShaderLocation(atmosphereShader, "atmosphereColor"),
                &atmosphereColor,
                SHADER_UNIFORM_VEC3
            );

            SetShaderValue(
                atmosphereShader,
                GetShaderLocation(atmosphereShader, "sunDir"),
                &sunDirection,
                SHADER_UNIFORM_VEC3
            );

            SetShaderValue(
                atmosphereShader,
                GetShaderLocation(atmosphereShader, "strength"),
                &strength,
                SHADER_UNIFORM_FLOAT
            );

            BeginBlendMode(BLEND_ADDITIVE);

            // Atmosphere should depth-test against the planet but not write depth.
            rlDisableDepthMask();

            const float atmosphereScale = GetAtmosphereShellScale(object);

            DrawModelEx(
                atmosphereModel,
                position,
                Vector3{ 1.0f, 0.0f, 0.0f },
                0.0f,
                Vector3{ radius * atmosphereScale, radius * atmosphereScale, radius * atmosphereScale },
                WHITE
            );

            rlEnableDepthMask();

            EndBlendMode();
        }
        static Model& GetCloudLayerModel(const GlobalObject& object)
        {
            static std::unordered_map<std::string, Model> cloudCache;

            auto found = cloudCache.find(object.id);
            if (found != cloudCache.end())
            {
                return found->second;
            }

            Model model = GenerateCloudLayerModel(object, 64);

            auto inserted = cloudCache.emplace(object.id, model);
            return inserted.first->second;
        }

        static bool ShouldDrawClouds(const GlobalObject& object)
        {
            if (!object.hasPlanetData)
            {
                return false;
            }

            const PlanetVisual visual = GeneratePlanetVisual(object);

            if (!visual.hasClouds)
            {
                return false;
            }

            // Gas/ice giants already render their visible atmosphere as bands.
            // A separate cloud shell over them looks like a transparent bubble in this prototype.
            return object.planetData.planetClass != PlanetClass::GasGiant &&
                object.planetData.planetClass != PlanetClass::IceGiant;
        }

        static void DrawCloudLayer(
            const GlobalObject& object,
            Vector3 position,
            float radius,
            float tilt,
            Vector3 sunDirection
        )
        {
            if (!ShouldDrawClouds(object))
            {
                return;
            }

            Model& cloudModel = GetCloudLayerModel(object);
            Shader& cloudShader = GetCloudShader();

            cloudModel.materials[0].shader = cloudShader;

            float cloudSeed = static_cast<float>(object.planetData.seed % 10000u) * 0.017f;

            SetShaderValue(
                cloudShader,
                GetShaderLocation(cloudShader, "cloudSeed"),
                &cloudSeed,
                SHADER_UNIFORM_FLOAT
            );

            SetShaderValue(
                cloudShader,
                GetShaderLocation(cloudShader, "sunDir"),
                &sunDirection,
                SHADER_UNIFORM_VEC3
            );

            BeginBlendMode(BLEND_ALPHA);

            // Important:
            // Leave backface culling ON.
            // Only disable depth writing.
            rlDisableDepthMask();

            const float cloudScale = GetCloudShellScale(object);

            DrawModelEx(
                cloudModel,
                position,
                Vector3{ 1.0f, 0.0f, 0.0f },
                tilt,
                Vector3{ radius * cloudScale, radius * cloudScale, radius * cloudScale },
                WHITE
            );

            rlEnableDepthMask();

            EndBlendMode();
        }
        static bool ShouldDrawOceanSpecular(const GlobalObject& object)
        {
            if (!object.hasPlanetData)
            {
                return false;
            }

            return object.planetData.planetClass == PlanetClass::OceanWorld;
        }

        static void DrawOceanSpecular(
            const GlobalObject& object,
            Vector3 position,
            float radius,
            float tilt
        )
        {
            if (!ShouldDrawOceanSpecular(object))
            {
                return;
            }

            // Temporary fake sun direction. Keep this matching the planet/cloud lighting.
            Vector3 sunDirection = Vector3Normalize(Vector3{ -0.55f, 0.22f, -0.80f });

            // Approximate view direction from sky body toward camera at origin.
            Vector3 viewDirection = Vector3Normalize(Vector3Negate(position));

            // Reflection direction from sun around the approximate planet-facing normal.
            // This is a cheap prototype highlight, not physically correct yet.
            Vector3 halfVector = Vector3Normalize(Vector3Add(sunDirection, viewDirection));

            Vector3 highlightOffset = Vector3Scale(halfVector, radius * 0.72f);
            Vector3 highlightPosition = Vector3Add(position, highlightOffset);

            float facing = Clamp(Vector3DotProduct(viewDirection, sunDirection) * 0.5f + 0.5f, 0.0f, 1.0f);

            float alpha = 0.08f + facing * 0.12f;

            BeginBlendMode(BLEND_ADDITIVE);
            rlDisableDepthMask();

            DrawSphere(
                highlightPosition,
                radius * 0.18f,
                Fade(Color{ 160, 205, 255, 255 }, alpha)
            );

            DrawSphere(
                highlightPosition,
                radius * 0.08f,
                Fade(Color{ 235, 245, 255, 255 }, alpha * 0.65f)
            );

            rlEnableDepthMask();
            EndBlendMode();

            (void)tilt;
        }
        static Model& GetGeneratedPlanetModel(const GlobalObject& object)
        {
            static std::unordered_map<std::string, Model> modelCache;

            auto found = modelCache.find(object.id);
            if (found != modelCache.end())
            {
                return found->second;
            }

            // Keep this <= 96 while PlanetMeshGenerator uses unsigned short indices.
            // 128+ creates more than 65k vertices and corrupts triangle indices.
            Model model = GenerateLowDetailPlanetModel(object, 96);

            auto inserted = modelCache.emplace(object.id, model);
            return inserted.first->second;
        }

        static void DrawTerrainPlanet(
            const GameWorld& world,
            const GlobalObject& object,
            const PlanetVisual& visual,
            Vector3 position,
            float radius
        )
        {
            DominantLight light = ComputeDominantLightForObject(world, position);
            Model& model = GetGeneratedPlanetModel(object);

            ApplyPlanetSurfaceShader(object, model, light);

            const float tilt = 20.0f + static_cast<float>(object.planetData.seed % 20u);

            DrawModelEx(
                model,
                position,
                Vector3{ 1.0f, 0.0f, 0.0f },
                tilt,
                Vector3{ radius, radius, radius },
                WHITE
            );

            //DrawOceanSpecular(object, position, radius, tilt);
            // Hold F9 to view the raw surface without cloud/atmosphere overlays.
            // This is useful because overlays can easily hide the actual lighting gradient.
            if (!IsKeyDown(KEY_F9))
            {
                DrawCloudLayer(object, position, radius, tilt, light.direction);
                DrawAtmosphereShell(object, visual, position, radius, light.direction);
            }
        }
        static Vector3 ToRenderDirection(DVec3 relative)
        {
            DVec3 direction = Normalize(relative);

            return Vector3{
                static_cast<float>(direction.x),
                static_cast<float>(direction.y),
                static_cast<float>(direction.z)
            };
        }

        static bool IsCelestialBody(GlobalObjectType type)
        {
            return type == GlobalObjectType::Sun ||
                type == GlobalObjectType::Planet;
        }

        struct SkyBodyDrawCommand
        {
            const GlobalObject* object = nullptr;
            Vector3 position{};
            float radius = 1.0f;
            PlanetVisual visual{};
            double globalDistance = 0.0;
            GlobalObjectType type = GlobalObjectType::Planet;
        };

        static float ComputeApparentSkyRadius(
            const GlobalObject& object,
            double globalDistance,
            float skyDistance
        )
        {
            if (globalDistance < 1.0)
            {
                return 1.0f;
            }

            float radius = static_cast<float>(
                object.visualRadius / globalDistance * static_cast<double>(skyDistance)
            );

            return Clamp(radius, 3.0f, 220.0f);
        }

        static void DrawDistantStarSystem3D(const GameWorld& world)
        {
            constexpr float skyDistance = SkyBodyRenderDistance;

            std::vector<SkyBodyDrawCommand> drawCommands;

            for (const GlobalObject& object : world.starSystem.objects)
            {
                if (!IsCelestialBody(object.type))
                {
                    continue;
                }

                DVec3 relative = object.position - world.globalPlayerPosition;
                double globalDistance = Length(relative);

                if (globalDistance < 1.0)
                {
                    continue;
                }

                Vector3 direction = ToRenderDirection(relative);
                Vector3 skyPos = Vector3Scale(direction, skyDistance);

                float radius = ComputeApparentSkyRadius(
                    object,
                    globalDistance,
                    skyDistance
                );

                drawCommands.push_back(SkyBodyDrawCommand{
                    &object,
                    skyPos,
                    radius,
                    GeneratePlanetVisual(object),
                    globalDistance,
                    object.type
                });
            }

            std::sort(
                drawCommands.begin(),
                drawCommands.end(),
                [](const SkyBodyDrawCommand& a, const SkyBodyDrawCommand& b)
                {
                    return a.globalDistance > b.globalDistance;
                }
            );

            // First draw suns and other non-planets.
            for (const SkyBodyDrawCommand& command : drawCommands)
            {
                if (command.type != GlobalObjectType::Planet)
                {
                    DrawSphere(command.position, command.radius, command.visual.baseColor);
                }
            }

            // Then draw planets so they visually sit in front.
            for (const SkyBodyDrawCommand& command : drawCommands)
            {
                if (command.type == GlobalObjectType::Planet && command.object != nullptr)
                {
                    DrawTerrainPlanet(world, *command.object, command.visual, command.position, command.radius);
                }
            }
        }
    }

    void DistantBodyRenderer::render(const GameWorld& world)
    {
        DrawDistantStarSystem3D(world);
    }
}