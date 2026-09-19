#include "rendering/DistantBodyRenderer.h"
#include "world/PlanetVisualGenerator.h"
#include "rendering/PlanetMeshGenerator.h"
#include "rendering/PlanetCloudGenerator.h"
#include "planet/PlanetSurfaceSampler.h"
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
        // One weather field for both visible clouds and their projected shadows.
        std::string WithWeatherField(const char* source)
        {
            std::string result(source);
            const std::string marker = "/* WEATHER_FIELD */";
            const auto at = result.find(marker);
            if (at != std::string::npos) result.replace(at, marker.size(), R"(
                float cloudDensity(vec3 dir, float seed) {
                    vec3 p = dir * 3.4 + vec3(seed);
                    float broad = noise(p);
                    float cells = noise(dir * 13.0 + vec3(seed * 1.71));
                    float edge = noise(dir * 47.0 + vec3(seed * 2.37));
                    return smoothstep(0.48, 0.72, broad * 0.67 + cells * 0.25 + edge * 0.08);
                }
            )");
            return result;
        }
        static float GetCloudShellScale(const GlobalObject& object)
        {
            if (!object.hasPlanetData)
            {
                return 1.03f;
            }

            switch (object.planetData.planetClass)
            {
            case PlanetClass::OceanWorld:
                return 1.006f;   // thin cloud layer above physically scaled terrain
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
            if (object.hasPlanetData && object.planetData.planetClass == PlanetClass::OceanWorld)
                return 1.012f;
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

            Mesh mesh = GenMeshSphere(1.0f, 128, 64);
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
        uniform vec3 cameraPosition;
        uniform float planetRadius;
        uniform vec3 atmosphereColor;
        uniform vec3 sunDir;
        uniform float strength;

        void main()
        {
            vec3 normal = normalize(fragNormal);

            // Camera is effectively at local origin for this sky/distant-body render.
            vec3 viewDir = normalize(cameraPosition - fragWorldPos);

            float viewDot = clamp(dot(normal, viewDir), 0.0, 1.0);

            // Fresnel rim: visible mainly at the edge, not across the whole sphere.
            float rim = pow(1.0 - viewDot, 2.0);

            // More atmosphere on the sunlit edge, but still faint on night edge.
            float day = smoothstep(-0.25, 0.45, dot(normal, normalize(sunDir)));
            vec3 cameraOffset = cameraPosition - planetCenter;
            float impact = length(cross(cameraOffset, -viewDir)) / planetRadius;
            float outerHeight = max(impact - 1.0, 0.0) / 0.012;
            float density = exp(-outerHeight * 3.5) * (1.0 - smoothstep(0.7, 1.0, outerHeight));
            float alpha = clamp((0.025 + rim * 0.55) * density * day * strength, 0.0, 0.60);

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
        out vec3 fragSurfaceDirection;
        uniform vec3 planetCenter;
        out vec4 fragVertexColor;

        uniform float planetSeed;
        uniform float bumpDetail;
        out vec3 fragBumpNormal;
        out float fragElevation;
        uniform float planetRadius;
        uniform float terrainUnitsPerRadius;

        void main()
        {
            vec4 worldPos = matModel * vec4(vertexPosition, 1.0);

            fragWorldPos = worldPos.xyz;
            fragSurfaceDirection = worldPos.xyz - planetCenter;
            fragNormal = normalize(mat3(matModel) * vertexNormal);
            // Use normals from the actual displaced terrain, shared by both LODs.
            if (dot(fragNormal, fragSurfaceDirection) < 0.0) fragNormal = -fragNormal;
            fragBumpNormal = fragNormal;
            fragElevation = (length(fragSurfaceDirection) / planetRadius - 1.0) * terrainUnitsPerRadius;
            fragVertexColor = vertexColor;

            gl_Position = mvp * vec4(vertexPosition, 1.0);
        }
        )";

            const char* fragmentShader = R"(
        #version 330

        in vec3 fragWorldPos;
        in vec3 fragNormal;
        in vec3 fragSurfaceDirection;
        uniform vec3 cameraPosition;
        in vec3 fragBumpNormal;
        uniform float groundDetail;
        in float fragElevation;
        in vec4 fragVertexColor;

        out vec4 finalColor;

        uniform float planetSeed;
        uniform vec3 sunDir;
        uniform vec3 sunColor;
        uniform float sunIntensity;
        uniform vec3 ambientColor;
        uniform float ambientIntensity;
        // 0 = unlit albedo, 1 = geometric normals, 2 = lit.
        uniform int lightingDebugMode;

        float hash(vec3 p)
        {
            uvec3 q = uvec3(ivec3(p));
            uint h = q.x * 374761393u ^ q.y * 668265263u ^ q.z * 2246822519u;
            h = (h ^ (h >> 13u)) * 1274126177u;
            h ^= h >> 16u;
            return float(h & 0x00ffffffu) / 16777215.0;
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
        /* WEATHER_FIELD */
        void main()
        {
            vec3 sphereNormal = normalize(fragNormal);
            vec3 dir = normalize(fragSurfaceDirection);

            vec3 normal = normalize(fragBumpNormal);

            vec3 sun = normalize(sunDir);
            vec3 viewDir = normalize(cameraPosition - fragWorldPos);

            // Continents / coastlines.
            float continents = fbm(dir * 2.15 + vec3(planetSeed)) +
                (noise(dir * 38.0 + vec3(planetSeed * 0.61)) - 0.5) * 0.012;
            float detail = fbm(dir * 16.0 + vec3(planetSeed * 1.73));
            float fine = fbm(dir * 42.0 + vec3(planetSeed * 2.19));

            // Lower thresholds = more land.
            // The previous 0.590/0.635 made this seed almost all ocean.
            float landRaw = continents;
            float landMask = smoothstep(0.535, 0.540, landRaw);

            // Coastal/shallow band just below land.
            float coastBand = smoothstep(0.528, 0.537, landRaw) * (1.0 - landMask);
            float shallowMask = coastBand;

            // Ocean colors: muted, but not black.
            vec3 deepOcean = vec3(0.010, 0.042, 0.095);
            vec3 midOcean = vec3(0.018, 0.105, 0.210);
            vec3 shallowOcean = vec3(0.030, 0.220, 0.245);

            float oceanVariation = fbm(dir * 4.0 + vec3(planetSeed * 0.41));
            vec3 oceanColor = mix(deepOcean, midOcean, oceanVariation * 0.65);
            oceanColor = mix(oceanColor, shallowOcean, shallowMask * 0.65);

            // Land material variation.
            float landDetail = detail * 0.70 + fine * 0.30;
            float dryness = fbm(dir * 5.0 + vec3(planetSeed * 0.91));
            float latitude = abs(dir.y);

            float elevation = max(fragElevation, 0.0);
            float slope = 1.0 - max(dot(normal, dir), 0.0);
            float rockMask = max(smoothstep(10.0, 25.0, elevation), smoothstep(0.08, 0.35, slope));
            float dryMask = smoothstep(0.35, 0.60, dryness) * (1.0 - smoothstep(0.55, 0.85, latitude));
            float coldMask = max(smoothstep(0.78, 0.95, latitude), smoothstep(29.0, 40.0, elevation) * smoothstep(0.28, 0.70, latitude));

            // Muted terrain palette.
            vec3 lushLand = vec3(0.045, 0.115, 0.055);
            vec3 temperateLand = vec3(0.18, 0.235, 0.10);
            vec3 dryLand = vec3(0.48, 0.34, 0.16);
            vec3 rockyLand = vec3(0.255, 0.245, 0.215);
            vec3 coldLand = vec3(0.58, 0.64, 0.66);

            vec3 landColor = mix(lushLand, temperateLand, smoothstep(0.25, 0.75, landDetail));
            landColor = mix(landColor, dryLand, dryMask * 0.90);
            landColor = mix(landColor, rockyLand, rockMask * 0.88);
            landColor = mix(landColor, coldLand, coldMask * 0.95);

            // Final surface blend.
            vec3 baseColor = mix(oceanColor, landColor, landMask);

            // Broad rock strata and soil pockets become visible only on approach.
            // Filter subpixel detail and avoid uncorrelated, high-contrast grain.
            if (groundDetail > 0.0 && landMask > 0.0)
            {
                float footprint = max(length(dFdx(dir)), length(dFdy(dir)));
                float rock = noise(dir * 180.0 + vec3(planetSeed * 0.43));
                float soil = noise(dir * 640.0 + vec3(planetSeed * 0.87));
                soil = mix(soil, 0.5, smoothstep(0.3, 1.2, footprint * 640.0));
                float strata = smoothstep(0.35, 0.65, rock * 0.8 + soil * 0.2);
                vec3 mineral = mix(vec3(0.82, 0.86, 0.90), vec3(1.10, 1.05, 0.94), strata);
                baseColor *= mix(vec3(1.0), mineral,
                    groundDetail * landMask * (1.0 - smoothstep(0.4, 1.4, footprint * 180.0)));
            }

            // Subtle coastal tint.
            vec3 coastTint = vec3(0.100, 0.290, 0.330);
            baseColor = mix(baseColor, coastTint, coastBand * 0.22);

            // Slight desaturation, but not too muddy.
            float gray = dot(baseColor, vec3(0.299, 0.587, 0.114));
            baseColor = mix(vec3(gray), baseColor, 0.82);

            // Debug modes deliberately stop before lighting so geography/material
            // problems can be separated from normal and illumination problems.
            if (lightingDebugMode == 0)
            {
                finalColor = vec4(clamp(baseColor, 0.0, 1.0), 1.0);
                return;
            }
            if (lightingDebugMode == 1)
            {
                finalColor = vec4(normal * 0.5 + 0.5, 1.0);
                return;
            }

            // Authoritative scene lighting. Geometry/material generation stays unlit;
            // illumination is evaluated every draw from the shared star state.
            float surfaceNdotL = max(dot(normal, sun), 0.0);
            float sphereNdotL = dot(dir, sun);
            float day = smoothstep(-0.03, 0.08, sphereNdotL);

            vec3 directLight = baseColor * sunColor * (sunIntensity * surfaceNdotL);
            vec3 indirectLight = baseColor * ambientColor * ambientIntensity;

            // Trace sunlight to the same cloud shell used for the visible weather.
            // Clouds attenuate direct sunlight, not the ambient term.
            float mu = dot(dir, sun);
            float cloudRay = -mu + sqrt(max(0.0, mu * mu + 1.006 * 1.006 - 1.0));
            float shadow = cloudDensity(normalize(dir + sun * cloudRay), planetSeed);
            directLight *= 1.0 - shadow * 0.32 * day;

            vec3 litColor = indirectLight + directLight;

            // Retain a restrained placeholder aerial-perspective cue until the
            // dedicated atmosphere-scattering milestone replaces it.
            float horizon = pow(1.0 - max(dot(dir, viewDir), 0.0), 3.0);
            litColor = mix(litColor, vec3(0.13, 0.28, 0.48), horizon * 0.12 * day);

            // Ocean response remains intentionally simple for now, but the direct
            // specular highlight is driven by the same star color/intensity.
            vec3 halfDir = normalize(sun + viewDir);

            // Small, filtered wave highlights provide scale without more noise octaves.
            float phase = dot(dir, vec3(1730.0, 910.0, 1310.0));
            float ripple = sin(phase) * sin(phase * 0.73 + dir.y * 270.0);
            ripple *= 1.0 - smoothstep(0.5, 2.0, fwidth(phase));
            litColor += sunColor * (0.004 * sunIntensity) * ripple * (1.0 - landMask) * day;

            float oceanSpec = pow(max(dot(normal, halfDir), 0.0), 96.0);
            oceanSpec *= (1.0 - landMask);
            oceanSpec *= day;

            float oceanFresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 4.0);
            oceanFresnel *= (1.0 - landMask);

            litColor += sunColor * oceanSpec * (0.55 * sunIntensity);
            litColor += vec3(0.05, 0.13, 0.24) * oceanFresnel * 0.16;

            finalColor = vec4(clamp(litColor, 0.0, 1.0), 1.0);
        }
        )";

            shader = LoadShaderFromMemory(vertexShader, WithWeatherField(fragmentShader).c_str());
            loaded = true;

            return shader;
        }
        static Shader& GetRockSurfaceShader()
        {
            static Shader shader{};
            if (shader.id != 0) return shader;
            const char* vertexShader = R"(
                #version 330
                in vec3 vertexPosition;
                in vec4 vertexColor;
                uniform mat4 mvp;
                out vec3 surfacePosition;
                out vec4 surfaceColor;
                void main() {
                    surfacePosition = vertexPosition * 2048.0;
                    surfaceColor = vertexColor;
                    gl_Position = mvp * vec4(vertexPosition, 1.0);
                }
            )";
            const char* fragmentShader = R"(
                #version 330
                in vec3 surfacePosition;
                in vec4 surfaceColor;
                out vec4 finalColor;
                float hash(vec3 p) {
                    p = fract(p * 0.1031);
                    p += dot(p, p.yzx + 33.33);
                    return fract((p.x + p.y) * p.z);
                }
                float noise(vec3 p) {
                    vec3 i = floor(p), f = fract(p);
                    f = f*f*(3.0 - 2.0*f);
                    return mix(mix(mix(hash(i), hash(i+vec3(1,0,0)), f.x),
                                   mix(hash(i+vec3(0,1,0)), hash(i+vec3(1,1,0)), f.x), f.y),
                               mix(mix(hash(i+vec3(0,0,1)), hash(i+vec3(1,0,1)), f.x),
                                   mix(hash(i+vec3(0,1,1)), hash(i+vec3(1,1,1)), f.x), f.y), f.z);
                }
                void main() {
                    // Object-space detail stays fixed to the ground during flight.
                    // Fade frequencies below a pixel to avoid distant shimmer.
                    float footprint = max(length(dFdx(surfacePosition)), length(dFdy(surfacePosition)));
                    float broad = noise(surfacePosition * 0.12);
                    float rock = mix(noise(surfacePosition * 0.8), 0.5,
                        smoothstep(0.4, 1.5, footprint * 0.8));
                    float grain = mix(noise(surfacePosition * 4.0), 0.5,
                        smoothstep(0.4, 1.5, footprint * 4.0));
                    float veins = smoothstep(0.02, 0.16, abs(rock - 0.5));
                    float texture = 0.72 + broad * 0.48 + rock * 0.26 + grain * 0.12;
                    texture *= mix(0.78, 1.0, veins);
                    vec3 mineral = mix(vec3(0.88, 0.94, 1.04), vec3(1.10, 1.02, 0.88), broad);
                    finalColor = vec4(clamp(surfaceColor.rgb * mineral * texture, 0.0, 1.0), 1.0);
                }
            )";
            shader = LoadShaderFromMemory(vertexShader, fragmentShader);
            return shader;
        }

        static void ApplyPlanetSurfaceShader(
            const GlobalObject& object,
            Model& model,
            float radius,
            Vector3 position,
            Vector3 cameraPosition,
            const SceneLighting& lighting
        )
        {
            if (!object.hasPlanetData ||
                object.planetData.planetClass != PlanetClass::OceanWorld)
            {
                if (object.hasPlanetData &&
                    object.planetData.planetClass != PlanetClass::GasGiant &&
                    object.planetData.planetClass != PlanetClass::IceGiant)
                {
                    model.materials[0].shader = GetRockSurfaceShader();
                }
                return;
            }

            model.materials[0].shader = DistantBodyRenderer::oceanSurfaceShader(
                object, lighting, position, radius, cameraPosition);
        }

        static Shader ConfigureOceanSurface(const GlobalObject& object,
            const SceneLighting& lighting,
            Vector3 position, float radius, Vector3 cameraPosition,
            int lightingDebugMode = 2)
        {
            Shader& oceanShader = GetOceanPlanetShader();

            // Safety: if the shader failed to compile/load, keep the default material.
            if (oceanShader.id == 0 || oceanShader.locs == nullptr)
            {
                return oceanShader;
            }

            const int seed = object.planetData.seed == 0
                ? PlanetSurfaceSampler::seedFromId(object.id)
                : static_cast<int>(object.planetData.seed);
            float planetSeed = static_cast<float>((seed % 10000 + 10000) % 10000) * 0.017f;
            SetShaderValue(oceanShader, GetShaderLocation(oceanShader, "planetRadius"), &radius, SHADER_UNIFORM_FLOAT);
            const float terrainUnits = PlanetSurfaceSampler::TerrainUnitsPerRadius;
            SetShaderValue(oceanShader, GetShaderLocation(oceanShader, "terrainUnitsPerRadius"),
                &terrainUnits, SHADER_UNIFORM_FLOAT);
            const float relativeAltitude = Vector3Distance(cameraPosition, position) / std::max(radius, 1.0f) - 1.0f;
            float groundDetail = Clamp((0.22f - relativeAltitude) / 0.18f, 0.0f, 1.0f);
            groundDetail = groundDetail * groundDetail * (3.0f - 2.0f * groundDetail);
            SetShaderValue(oceanShader, GetShaderLocation(oceanShader, "groundDetail"), &groundDetail, SHADER_UNIFORM_FLOAT);
            // Sky and orbital planets use comparable render distances. Avoid
            // expensive vertex noise for small bodies and fade it in on approach.
            float bumpDetail = Clamp((radius - 80.0f) / 100.0f, 0.0f, 1.0f);
            Vector3 sunDirection = GetLightDirectionAt(lighting, object.position);

            SetShaderValue(oceanShader, GetShaderLocation(oceanShader, "bumpDetail"),
                &bumpDetail, SHADER_UNIFORM_FLOAT);
            SetShaderValue(oceanShader, GetShaderLocation(oceanShader, "planetCenter"),
                &position, SHADER_UNIFORM_VEC3);
            SetShaderValue(oceanShader, GetShaderLocation(oceanShader, "cameraPosition"),
                &cameraPosition, SHADER_UNIFORM_VEC3);

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

            SetShaderValue(
                oceanShader,
                GetShaderLocation(oceanShader, "sunColor"),
                &lighting.starColor,
                SHADER_UNIFORM_VEC3
            );
            SetShaderValue(
                oceanShader,
                GetShaderLocation(oceanShader, "sunIntensity"),
                &lighting.starIntensity,
                SHADER_UNIFORM_FLOAT
            );
            SetShaderValue(
                oceanShader,
                GetShaderLocation(oceanShader, "ambientColor"),
                &lighting.ambientColor,
                SHADER_UNIFORM_VEC3
            );
            SetShaderValue(
                oceanShader,
                GetShaderLocation(oceanShader, "ambientIntensity"),
                &lighting.ambientIntensity,
                SHADER_UNIFORM_FLOAT
            );
            SetShaderValue(
                oceanShader,
                GetShaderLocation(oceanShader, "lightingDebugMode"),
                &lightingDebugMode,
                SHADER_UNIFORM_INT
            );
            return oceanShader;
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
            uvec3 q = uvec3(ivec3(p));
            uint h = q.x * 374761393u ^ q.y * 668265263u ^ q.z * 2246822519u;
            h = (h ^ (h >> 13u)) * 1274126177u;
            h ^= h >> 16u;
            return float(h & 0x00ffffffu) / 16777215.0;
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

        /* WEATHER_FIELD */
        void main()
        {
            vec3 dir = normalize(fragDir);

            float mask = cloudDensity(dir, cloudSeed);
            // Light clouds using the authoritative scene-star direction.
            float day = smoothstep(-0.15, 0.35, dot(dir, normalize(sunDir)));

            // Clouds should be much dimmer on the night side.
            float alpha = (1.0 - exp(-mask * 3.0)) * mix(0.18, 0.90, day);

            if (alpha < 0.006)
            {
                discard;
            }
            // Slightly brighter on day side, bluish-gray on night side.
            vec3 nightColor = vec3(0.06, 0.09, 0.14);
            vec3 dayColor = vec3(0.92, 0.96, 1.0);
            vec3 color = mix(nightColor, dayColor, day) * mix(0.78, 1.0, mask);

            finalColor = vec4(color, alpha);
        }
        )";

            shader = LoadShaderFromMemory(vertexShader, WithWeatherField(fragmentShader).c_str());
            loaded = true;

            return shader;
        }
        static void DrawAtmosphereShell(
            const GlobalObject& object,
            const PlanetVisual& visual,
            Vector3 position,
            float radius,
            Vector3 cameraPosition,
            const SceneLighting& lighting
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

            Vector3 sunDirection = GetLightDirectionAt(lighting, object.position);

            float strength = Clamp(visual.atmosphereStrength * 0.55f, 0.08f, 0.55f);
            strength = Clamp(visual.atmosphereStrength, 0.0f, 1.0f);
            SetShaderValue(atmosphereShader, GetShaderLocation(atmosphereShader, "cameraPosition"), &cameraPosition, SHADER_UNIFORM_VEC3);
            SetShaderValue(atmosphereShader, GetShaderLocation(atmosphereShader, "planetRadius"), &radius, SHADER_UNIFORM_FLOAT);

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

            BeginBlendMode(BLEND_ALPHA);

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
            float tilt,
            const SceneLighting& lighting
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
            Vector3 sunDirection = GetLightDirectionAt(lighting, object.position);

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
            float tilt,
            const SceneLighting& lighting
        )
        {
            if (!ShouldDrawOceanSpecular(object))
            {
                return;
            }

            // Use the same scene-star direction as the planet and cloud shaders.
            Vector3 sunDirection = GetLightDirectionAt(lighting, object.position);

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
            float radius,
            const SceneLighting& lighting,
            Vector3 cameraPosition = {}
        )
        {
            Model& model = GetGeneratedPlanetModel(object);

            ApplyPlanetSurfaceShader(object, model, radius, position, cameraPosition, lighting);

            const float tilt = object.planetData.planetClass == PlanetClass::OceanWorld
                ? 0.0f : 20.0f + static_cast<float>(object.planetData.seed % 20u);

            DrawModelEx(
                model,
                position,
                Vector3{ 1.0f, 0.0f, 0.0f },
                tilt,
                Vector3{ radius, radius, radius },
                WHITE
            );

            //DrawOceanSpecular(object, position, radius, tilt, lighting);
            DrawCloudLayer(object, position, radius, tilt, lighting);
            DrawAtmosphereShell(object, visual, position, radius, cameraPosition, lighting);
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

            for (int objectIndex = 0; objectIndex < static_cast<int>(world.starSystem.objects.size()); ++objectIndex)
            {
                const GlobalObject& object = world.starSystem.objects[objectIndex];

                const bool isActiveLocalPlanet =
                    objectIndex == world.planetTransition.closestPlanetIndex &&
                    world.planetTransition.mode != PlanetRenderMode::Distant;

                if (isActiveLocalPlanet)
                {
                    continue;
                }

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
                    DrawTerrainPlanet(*command.object, command.visual, command.position, command.radius, world.lighting);
                }
            }
        }
    }

    void DistantBodyRenderer::render(const GameWorld& world)
    {
        DrawDistantStarSystem3D(world);
    }

    Shader DistantBodyRenderer::oceanSurfaceShader(const GlobalObject& object,
        const SceneLighting& lighting,
        Vector3 center, float radius, Vector3 cameraPosition,
        int lightingDebugMode)
    {
        return ConfigureOceanSurface(object, lighting, center, radius, cameraPosition,
            lightingDebugMode);
    }

    void DistantBodyRenderer::renderLocalPlanet(
        const GlobalObject& object,
        const SceneLighting& lighting,
        Vector3 position,
        float radius,
        float atmosphereMultiplier,
        Vector3 cameraPosition, bool drawSurface, bool drawLayers)
    {
        PlanetVisual visual = GeneratePlanetVisual(object);

        visual.atmosphereStrength *= atmosphereMultiplier;

        if (object.type == GlobalObjectType::Sun)
        {
            DrawSphere(position, radius, visual.baseColor);
            return;
        }

        if (drawSurface && drawLayers) {
            DrawTerrainPlanet(object, visual, position, radius, lighting, cameraPosition);
        } else {
            if (drawSurface) {
                Model& model = GetGeneratedPlanetModel(object);
                ApplyPlanetSurfaceShader(object, model, radius, position, cameraPosition, lighting);
                const float tilt = object.planetData.planetClass == PlanetClass::OceanWorld
                    ? 0.0f : 20.0f + static_cast<float>(object.planetData.seed % 20u);
                DrawModelEx(model, position, {1,0,0}, tilt, {radius,radius,radius}, WHITE);
            }
            if (drawLayers) {
                DrawCloudLayer(object, position, radius, 0.0f, lighting);
                DrawAtmosphereShell(object, visual, position, radius, cameraPosition, lighting);
            }
        }
    }
}
