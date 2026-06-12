#include "rendering/MaterialTestSceneRenderer.h"
#include <raylib.h>
#include <raymath.h>
namespace SpaceSim
{
    void MaterialTestSceneRenderer::render(
        LitMeshRenderer& litMeshRenderer,
        const Camera3D& camera,
        RenderDebugView debugView
    )
    {
        LightingEnvironment lighting{};

        // Direction from sphere surface toward the sun.
        lighting.sun.directionToLight = Vector3Normalize(Vector3{ -0.6f, 0.35f, -0.72f });
        lighting.sun.color = Vector3{ 1.0f, 0.96f, 0.88f };
        lighting.sun.intensity = 1.35f;
        lighting.ambientIntensity = 0.025f;

        MaterialParams matteGray{};
        matteGray.albedo = Vector3{ 0.55f, 0.55f, 0.55f };
        matteGray.roughness = 0.95f;
        matteGray.specularStrength = 0.02f;
        matteGray.diffuseStrength = 1.0f;

        MaterialParams glossyBlue{};
        glossyBlue.albedo = Vector3{ 0.02f, 0.12f, 0.42f };
        glossyBlue.roughness = 0.12f;
        glossyBlue.specularStrength = 1.4f;
        glossyBlue.diffuseStrength = 0.25f;

        MaterialParams roughGreen{};
        roughGreen.albedo = Vector3{ 0.18f, 0.45f, 0.18f };
        roughGreen.roughness = 0.85f;
        roughGreen.specularStrength = 0.08f;
        roughGreen.diffuseStrength = 1.0f;

        MaterialParams shinyWhite{};
        shinyWhite.albedo = Vector3{ 0.85f, 0.85f, 0.82f };
        shinyWhite.roughness = 0.05f;
        shinyWhite.specularStrength = 1.8f;
        shinyWhite.diffuseStrength = 0.45f;

        litMeshRenderer.drawSphere(
            Vector3{ -9.0f, 0.0f, 18.0f },
            2.0f,
            matteGray,
            lighting,
            camera,
            debugView
        );

        litMeshRenderer.drawSphere(
            Vector3{ -3.0f, 0.0f, 18.0f },
            2.0f,
            glossyBlue,
            lighting,
            camera,
            debugView
        );

        litMeshRenderer.drawSphere(
            Vector3{ 3.0f, 0.0f, 18.0f },
            2.0f,
            roughGreen,
            lighting,
            camera,
            debugView
        );

        litMeshRenderer.drawSphere(
            Vector3{ 9.0f, 0.0f, 18.0f },
            2.0f,
            shinyWhite,
            lighting,
            camera,
            debugView
        );

        DrawText("F10 material test scene", 20, GetScreenHeight() - 90, 20, YELLOW);
        DrawText("F1 final | F2 albedo | F3 normals | F4 NdotL | F5 diffuse | F6 specular | F7 roughness", 20, GetScreenHeight() - 65, 20, RAYWHITE);
    }
}