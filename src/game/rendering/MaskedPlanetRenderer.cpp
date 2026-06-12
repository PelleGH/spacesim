#include "rendering/MaskedPlanetRenderer.h"

#include <raylib.h>
#include <raymath.h>

#include <utility>

namespace SpaceSim
{
    static Shader LoadMaskedPlanetShader()
    {
        const char* vertexShader = R"(
#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

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
uniform sampler2D materialTex;
uniform sampler2D normalTex;

uniform vec3 cameraPosition;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;

uniform vec3 ambientColor;
uniform float ambientIntensity;

uniform int debugView;

const float PI = 3.14159265358979323846;

vec2 dirToUv(vec3 dir)
{
    dir = normalize(dir);

    float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    float v = acos(clamp(dir.y, -1.0, 1.0)) / PI;

    return vec2(u, v);
}

float hash31(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float valueNoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));

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
        total += valueNoise(p) * amp;
        p *= 2.03;
        amp *= 0.5;
    }

    return total;
}

void main()
{
    vec3 sphereN = normalize(fragWorldNormal);
    vec3 objectN = normalize(fragObjectNormal);

    vec3 V = normalize(cameraPosition - fragWorldPos);
    vec3 L = normalize(lightDirection);

    vec2 uv = dirToUv(objectN);
    uv.x = fract(uv.x);

    vec3 albedo = texture(albedoTex, uv).rgb;
    vec4 materialSample = texture(materialTex, uv);
    vec3 normalSample = texture(normalTex, uv).rgb * 2.0 - 1.0;

    float waterMask = materialSample.r;
    float landMask = 1.0 - waterMask;
    float heightInfo = materialSample.a;

    // Build a simple tangent frame around the sphere normal.
    // This is good enough for distant/orbit visuals.
    vec3 tangentBase = abs(sphereN.y) < 0.95
        ? vec3(0.0, 1.0, 0.0)
        : vec3(1.0, 0.0, 0.0);

    vec3 tangent = normalize(cross(tangentBase, sphereN));
    vec3 bitangent = normalize(cross(sphereN, tangent));

    vec3 detailN = normalize(
        tangent * normalSample.x +
        bitangent * normalSample.y +
        sphereN * normalSample.z
    );

    float normalStrength = mix(0.03, 0.65, landMask) * heightInfo;
    vec3 N = normalize(mix(sphereN, detailN, normalStrength));

    vec3 H = normalize(L + V);

    // Keep broad day/night based mostly on the actual sphere.
    // Detail normals should add texture, not destroy planet-scale lighting.
    float broadNdotL = max(dot(sphereN, L), 0.0);
    float detailNdotL = max(dot(N, L), 0.0);
    float ndotl = mix(broadNdotL, detailNdotL, 0.35);

    float roughness = clamp(materialSample.g, 0.02, 1.0);
    float specularStrength = materialSample.b;

    float diffuseStrength = mix(0.18, 1.0, landMask);

    vec3 ambient = albedo * ambientColor * ambientIntensity;

    vec3 diffuse =
        albedo *
        ndotl *
        lightColor *
        lightIntensity *
        diffuseStrength;

    float shininess = mix(256.0, 8.0, roughness);

    float specAmount = pow(max(dot(N, H), 0.0), shininess);
    specAmount *= specularStrength;
    specAmount *= ndotl;

    vec3 specularColor = mix(lightColor, vec3(1.0, 0.98, 0.90), waterMask * 0.75);
    vec3 specular = specularColor * lightIntensity * specAmount;

    float broadWaterSheen = pow(max(dot(N, H), 0.0), 18.0);
    float sheenNoise = 0.75 + fbm(objectN * 38.0 + vec3(2.0, 9.0, 4.0)) * 0.35;

    vec3 waterSheen =
        specularColor *
        broadWaterSheen *
        waterMask *
        ndotl *
        sheenNoise *
        0.18;

    specular += waterSheen;

    vec3 color = ambient + diffuse + specular;

    if (debugView == 1)
    {
        finalColor = vec4(albedo, 1.0);
        return;
    }

    if (debugView == 2)
    {
        finalColor = vec4(N * 0.5 + 0.5, 1.0);
        return;
    }

    if (debugView == 3)
    {
        finalColor = vec4(vec3(ndotl), 1.0);
        return;
    }

    if (debugView == 4)
    {
        finalColor = vec4(diffuse, 1.0);
        return;
    }

    if (debugView == 5)
    {
        finalColor = vec4(specular, 1.0);
        return;
    }

    if (debugView == 6)
    {
        finalColor = vec4(vec3(roughness), 1.0);
        return;
    }

    if (debugView == 7)
    {
        // F8 material mask:
        // red   = water
        // green = land
        // blue  = specular strength
        finalColor = vec4(
            waterMask,
            landMask,
            clamp(specularStrength / 0.55, 0.0, 1.0),
            1.0
        );
        return;
    }

    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));

    finalColor = vec4(color, 1.0);
}
)";

        return LoadShaderFromMemory(vertexShader, fragmentShader);
    }

    MaskedPlanetRenderer::MaskedPlanetRenderer()
    {
        m_shader = LoadMaskedPlanetShader();
        if (m_shader.id != 0)
        {
            m_shader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(m_shader, "albedoTex");
            m_shader.locs[SHADER_LOC_MAP_SPECULAR] = GetShaderLocation(m_shader, "materialTex");
            m_shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(m_shader, "normalTex");
        }
        Mesh sphereMesh = GenMeshSphere(1.0f, 96, 48);
        m_sphereModel = LoadModelFromMesh(sphereMesh);

        m_sphereModel.materials[0].shader = m_shader;

        m_ready = m_shader.id != 0;
    }
    MaskedPlanetRenderer::~MaskedPlanetRenderer()
    {
        if (!IsWindowReady())
        {
            return;
        }

        for (auto& entry : m_surfaceMapCache)
        {
            UnloadPlanetSurfaceMaps(entry.second);
        }

        m_surfaceMapCache.clear();

        if (m_sphereModel.meshCount > 0)
        {
            UnloadModel(m_sphereModel);
        }

        if (m_shader.id != 0)
        {
            UnloadShader(m_shader);
        }
    }
    PlanetSurfaceMaps& MaskedPlanetRenderer::getSurfaceMaps(const GlobalObject& object)
    {
        auto found = m_surfaceMapCache.find(object.id);

        if (found != m_surfaceMapCache.end())
        {
            return found->second;
        }

        PlanetSurfaceMaps maps = GeneratePlanetSurfaceMaps(
            object,
            1024,
            512
        );

        auto inserted = m_surfaceMapCache.emplace(object.id, std::move(maps));
        return inserted.first->second;
    }

    void MaskedPlanetRenderer::drawSphere(
        const GlobalObject& object,
        Vector3 position,
        float radius,
        const LightingEnvironment& lighting,
        const Camera3D& camera,
        RenderDebugView debugView
    )
    {
        if (!m_ready)
        {
            DrawSphere(position, radius, RED);
            return;
        }

        m_sphereModel.materials[0].shader = m_shader;
        PlanetSurfaceMaps& maps = getSurfaceMaps(object);

        m_sphereModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = maps.albedo;
        m_sphereModel.materials[0].maps[MATERIAL_MAP_SPECULAR].texture = maps.material;
        m_sphereModel.materials[0].maps[MATERIAL_MAP_NORMAL].texture = maps.normal;
        Vector3 lightDir = Vector3Normalize(lighting.sun.directionToLight);

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "cameraPosition"),
            &camera.position,
            SHADER_UNIFORM_VEC3
        );

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "lightDirection"),
            &lightDir,
            SHADER_UNIFORM_VEC3
        );

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "lightColor"),
            &lighting.sun.color,
            SHADER_UNIFORM_VEC3
        );

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "lightIntensity"),
            &lighting.sun.intensity,
            SHADER_UNIFORM_FLOAT
        );

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "ambientColor"),
            &lighting.ambientColor,
            SHADER_UNIFORM_VEC3
        );

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "ambientIntensity"),
            &lighting.ambientIntensity,
            SHADER_UNIFORM_FLOAT
        );

        int debugValue = static_cast<int>(debugView);

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "debugView"),
            &debugValue,
            SHADER_UNIFORM_INT
        );

        DrawModelEx(
            m_sphereModel,
            position,
            Vector3{ 1.0f, 0.0f, 0.0f },
            0.0f,
            Vector3{ radius, radius, radius },
            WHITE
        );
    }
}