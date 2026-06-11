#include "rendering/DistantBodyRenderer.h"
#include "world/PlanetVisualGenerator.h"
#include "rendering/PlanetMeshGenerator.h"
#include "rendering/PlanetCloudGenerator.h"
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
                return 1.055f;   // needs more clearance because terrain + cloud coverage
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
        )";

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
            float rim = pow(1.0 - viewDot, 6.0);

            // More atmosphere on the sunlit edge, but still faint on night edge.
            float day = smoothstep(-0.25, 0.45, dot(normal, normalize(sunDir)));
            float alpha = rim * mix(0.0, 0.32, day) * strength;

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

        static Shader& GetOceanPlanetShader()
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
        out vec3 fragNormal;
        out vec4 fragVertexColor;

        void main()
        {
            vec4 worldPos = matModel * vec4(vertexPosition, 1.0);

            fragWorldPos = worldPos.xyz;
            fragNormal = normalize(mat3(matModel) * vertexNormal);
            fragVertexColor = vertexColor;

            gl_Position = mvp * vec4(vertexPosition, 1.0);
        }
        )";

            const char* fragmentShader = R"(
        #version 330

        in vec3 fragWorldPos;
        in vec3 fragNormal;
        in vec4 fragVertexColor;

        out vec4 finalColor;

        uniform float planetSeed;
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

            for (int i = 0; i < 6; ++i)
            {
                total += noise(p) * amp;
                p *= 2.03;
                amp *= 0.5;
            }

            return total;
        }
        float terrainHeight(vec3 dir, float seed)
        {
            float continents = fbm(dir * 2.15 + vec3(seed));
            float detail = fbm(dir * 10.0 + vec3(seed * 1.73));
            float fine = fbm(dir * 24.0 + vec3(seed * 2.19));

            float landMask = smoothstep(0.535, 0.600, continents);

            float mountains = smoothstep(0.68, 0.95, detail);
            float roughness = fine * 0.18 + mountains * 0.55;

            float coastFade = smoothstep(0.600, 0.670, continents);

            return landMask * coastFade * roughness;
        }

        vec3 perturbNormal(vec3 dir, float seed)
        {
            vec3 up = abs(dir.y) < 0.95 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);

            vec3 tangent = normalize(cross(up, dir));
            vec3 bitangent = normalize(cross(dir, tangent));

            float e = 0.006;

            float h0 = terrainHeight(dir, seed);
            float hx = terrainHeight(normalize(dir + tangent * e), seed);
            float hy = terrainHeight(normalize(dir + bitangent * e), seed);

            float strength = 0.055;

            vec3 bumped = normalize(dir - tangent * (hx - h0) * strength - bitangent * (hy - h0) * strength);

            return bumped;
        }
        void main()
        {
            vec3 sphereNormal = normalize(fragNormal);
            vec3 dir = sphereNormal;

            vec3 normal = perturbNormal(dir, planetSeed);

            vec3 sun = normalize(sunDir);
            vec3 viewDir = normalize(-fragWorldPos);

            // Continents / coastlines.
            float continents = fbm(dir * 2.15 + vec3(planetSeed));
            float detail = fbm(dir * 16.0 + vec3(planetSeed * 1.73));
            float fine = fbm(dir * 42.0 + vec3(planetSeed * 2.19));

            // Lower thresholds = more land.
            // The previous 0.590/0.635 made this seed almost all ocean.
            float landRaw = continents;
            float landMask = smoothstep(0.535, 0.600, landRaw);

            // Coastal/shallow band just below land.
            float coastBand = smoothstep(0.485, 0.545, landRaw) * (1.0 - landMask);
            float shallowMask = coastBand;

            // Ocean colors: muted, but not black.
            vec3 deepOcean = vec3(0.012, 0.075, 0.190);
            vec3 midOcean = vec3(0.028, 0.145, 0.340);
            vec3 shallowOcean = vec3(0.075, 0.255, 0.350);

            float oceanVariation = fbm(dir * 4.0 + vec3(planetSeed * 0.41));
            vec3 oceanColor = mix(deepOcean, midOcean, oceanVariation * 0.25);
            oceanColor = mix(oceanColor, shallowOcean, shallowMask * 0.85);

            // Land material variation.
            float landDetail = detail * 0.70 + fine * 0.30;
            float dryness = fbm(dir * 5.0 + vec3(planetSeed * 0.91));
            float latitude = abs(dir.y);

            float rockMask = smoothstep(0.68, 0.90, detail);
            float dryMask = smoothstep(0.58, 0.86, dryness);
            float coldMask = smoothstep(0.72, 0.96, latitude);

            // Muted terrain palette.
            vec3 lushLand = vec3(0.075, 0.170, 0.075);
            vec3 temperateLand = vec3(0.165, 0.230, 0.115);
            vec3 dryLand = vec3(0.320, 0.275, 0.165);
            vec3 rockyLand = vec3(0.255, 0.245, 0.215);
            vec3 coldLand = vec3(0.430, 0.450, 0.410);

            vec3 landColor = mix(lushLand, temperateLand, smoothstep(0.25, 0.75, landDetail));
            landColor = mix(landColor, dryLand, dryMask * 0.45);
            landColor = mix(landColor, rockyLand, rockMask * 0.35);
            landColor = mix(landColor, coldLand, coldMask * 0.35);

            // Final surface blend.
            vec3 baseColor = mix(oceanColor, landColor, landMask);

            // Subtle coastal tint.
            vec3 coastTint = vec3(0.100, 0.290, 0.330);
            baseColor = mix(baseColor, coastTint, coastBand * 0.22);

            // Slight desaturation, but not too muddy.
            float gray = dot(baseColor, vec3(0.299, 0.587, 0.114));
            baseColor = mix(vec3(gray), baseColor, 0.82);

            // Soft sunlight.
            float ndotl = dot(normal, sun);
            float day = smoothstep(-0.20, 0.42, ndotl);

            float ambient = 0.30;
            float sunlight = 0.95;
            vec3 litColor = baseColor * (ambient + day * sunlight);

            // Gentle exposure lift. Keeps the night side readable without making it flat.
            litColor = litColor * 1.18;

            // Fake ocean specular. Only on water, mostly on day side.
            vec3 halfDir = normalize(sun + viewDir);

            float oceanSpec = pow(max(dot(normal, halfDir), 0.0), 96.0);
            oceanSpec *= (1.0 - landMask);
            oceanSpec *= day;

            float oceanFresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 4.0);
            oceanFresnel *= (1.0 - landMask) * day;

            litColor += vec3(0.50, 0.68, 0.90) * oceanSpec * 0.55;
            litColor += vec3(0.05, 0.13, 0.24) * oceanFresnel * 0.20;

            finalColor = vec4(clamp(litColor, 0.0, 1.0), 1.0);
        }
        )";

            shader = LoadShaderFromMemory(vertexShader, fragmentShader);
            loaded = true;

            return shader;
        }
        static void ApplyPlanetSurfaceShader(
            const GlobalObject& object,
            Model& model
        )
        {
            // Only Ocean World uses the new shader for now.
            // Other planet types should keep their original/default material shader.
            if (!object.hasPlanetData ||
                object.planetData.planetClass != PlanetClass::OceanWorld)
            {
                return;
            }

            Shader& oceanShader = GetOceanPlanetShader();

            // Safety: if the shader failed to compile/load, keep the default material.
            if (oceanShader.id == 0 || oceanShader.locs == nullptr)
            {
                return;
            }

            model.materials[0].shader = oceanShader;

            float planetSeed = static_cast<float>(object.planetData.seed % 10000u) * 0.017f;
            Vector3 sunDirection = Vector3Normalize(Vector3{ -0.55f, 0.22f, -0.80f });

            SetShaderValue(
                oceanShader,
                GetShaderLocation(oceanShader, "planetSeed"),
                &planetSeed,
                SHADER_UNIFORM_FLOAT
            );

            SetShaderValue(
                oceanShader,
                GetShaderLocation(oceanShader, "sunDir"),
                &sunDirection,
                SHADER_UNIFORM_VEC3
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

            float cloud = broad * 0.58 + medium * 0.32 + fine * 0.10;

            // Patch coverage. Raise these numbers for fewer clouds.
            float mask = smoothstep(0.52, 0.74, cloud);

            // Break up the edges so they are not solid white blobs.
            float breakup = smoothstep(0.32, 0.72, medium + fine * 0.35);
            mask *= breakup;

            // Light clouds by the same fake sun direction used by the planet.
            float day = smoothstep(-0.15, 0.35, dot(dir, normalize(sunDir)));

            // Clouds should be much dimmer on the night side.
            float alpha = mask * mix(0.03, 0.52, day);

            if (alpha < 0.006)
            {
                discard;
            }
            // Slightly brighter on day side, bluish-gray on night side.
            vec3 nightColor = vec3(0.38, 0.46, 0.56);
            vec3 dayColor = vec3(0.92, 0.96, 1.0);
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
            float radius
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

            Vector3 sunDirection = Vector3Normalize(Vector3{ -0.55f, 0.22f, -0.80f });

            float strength = Clamp(visual.atmosphereStrength * 0.55f, 0.08f, 0.55f);

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

            return object.planetData.planetClass == PlanetClass::OceanWorld;
        }

        static void DrawCloudLayer(
            const GlobalObject& object,
            Vector3 position,
            float radius,
            float tilt
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
            Vector3 sunDirection = Vector3Normalize(Vector3{ -0.55f, 0.22f, -0.80f });

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

            Model model = GenerateLowDetailPlanetModel(object, 96);

            auto inserted = modelCache.emplace(object.id, model);
            return inserted.first->second;
        }

        static void DrawTerrainPlanet(
            const GlobalObject& object,
            const PlanetVisual& visual,
            Vector3 position,
            float radius
        )
        {
            Model& model = GetGeneratedPlanetModel(object);

            ApplyPlanetSurfaceShader(object, model);

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
            DrawCloudLayer(object, position, radius, tilt);
            DrawAtmosphereShell(object, visual, position, radius);
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
            constexpr float skyDistance = 900.0f;

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
                    DrawTerrainPlanet(*command.object, command.visual, command.position, command.radius);
                }
            }
        }
    }

    void DistantBodyRenderer::render(const GameWorld& world)
    {
        DrawDistantStarSystem3D(world);
    }
}