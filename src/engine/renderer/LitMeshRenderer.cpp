#include "renderer/LitMeshRenderer.h"

#include <raymath.h>

namespace SpaceSim
{
    static Shader LoadLitMeshShader()
    {
        const char* vertexShader = R"(
#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragWorldPos;
out vec3 fragWorldNormal;

void main()
{
    vec4 worldPos = matModel * vec4(vertexPosition, 1.0);

    fragWorldPos = worldPos.xyz;
    fragWorldNormal = normalize(mat3(matModel) * vertexNormal);

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

        const char* fragmentShader = R"(
#version 330

in vec3 fragWorldPos;
in vec3 fragWorldNormal;

out vec4 finalColor;

uniform vec3 cameraPosition;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;

uniform vec3 ambientColor;
uniform float ambientIntensity;

uniform vec3 materialAlbedo;
uniform float materialRoughness;
uniform float materialSpecularStrength;
uniform float materialDiffuseStrength;

uniform int debugView;

void main()
{
    vec3 N = normalize(fragWorldNormal);

    // Direction from shaded point toward camera.
    vec3 V = normalize(cameraPosition - fragWorldPos);

    // Direction from shaded point toward sun/light.
    vec3 L = normalize(lightDirection);

    vec3 H = normalize(L + V);

    float ndotl = max(dot(N, L), 0.0);

    vec3 albedo = materialAlbedo;

    vec3 ambient = albedo * ambientColor * ambientIntensity;

    vec3 diffuse =
        albedo *
        ndotl *
        lightColor *
        lightIntensity *
        materialDiffuseStrength;

    // Simple Blinn-Phong.
    // roughness 0 -> high shininess
    // roughness 1 -> low shininess
    float roughness = clamp(materialRoughness, 0.02, 1.0);
    float shininess = mix(256.0, 8.0, roughness);

    float specAmount = pow(max(dot(N, H), 0.0), shininess);
    specAmount *= materialSpecularStrength;
    specAmount *= ndotl;

    vec3 specular = lightColor * lightIntensity * specAmount;

    vec3 color = ambient + diffuse + specular;

    // Debug views.
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

    // Basic gamma correction.
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));

    finalColor = vec4(color, 1.0);
}
)";

        return LoadShaderFromMemory(vertexShader, fragmentShader);
    }

    LitMeshRenderer::LitMeshRenderer()
    {
        m_shader = LoadLitMeshShader();

        Mesh sphereMesh = GenMeshSphere(1.0f, 64, 32);
        m_sphereModel = LoadModelFromMesh(sphereMesh);

        m_sphereModel.materials[0].shader = m_shader;

        m_ready = m_shader.id != 0;
    }

    LitMeshRenderer::~LitMeshRenderer()
    {
        if (IsWindowReady())
        {
            if (m_sphereModel.meshCount > 0)
            {
                UnloadModel(m_sphereModel);
            }

            if (m_shader.id != 0)
            {
                UnloadShader(m_shader);
            }
        }
    }

    void LitMeshRenderer::drawSphere(
        Vector3 position,
        float radius,
        const MaterialParams& material,
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

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "materialAlbedo"),
            &material.albedo,
            SHADER_UNIFORM_VEC3
        );

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "materialRoughness"),
            &material.roughness,
            SHADER_UNIFORM_FLOAT
        );

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "materialSpecularStrength"),
            &material.specularStrength,
            SHADER_UNIFORM_FLOAT
        );

        SetShaderValue(
            m_shader,
            GetShaderLocation(m_shader, "materialDiffuseStrength"),
            &material.diffuseStrength,
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