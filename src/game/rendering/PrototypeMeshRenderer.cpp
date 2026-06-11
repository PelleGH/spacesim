#include "rendering/PrototypeMeshRenderer.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

namespace SpaceSim
{
    void PrototypeMeshRenderer::drawShip(Color color) const
    {
        DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, 0.4f, 2.0f, color);
        DrawCubeWires(Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, 0.4f, 2.0f, WHITE);
        DrawCube(Vector3{ 0.0f, 0.0f, 1.25f }, 0.35f, 0.25f, 0.35f, RED);
    }

    void PrototypeMeshRenderer::drawSatellite(Color panelColor) const
    {
        Vector3 satellitePos{ 0.0f, 0.0f, 0.0f };

        DrawCube(satellitePos, 8.0f, 4.0f, 4.0f, LIGHTGRAY);
        DrawCubeWires(satellitePos, 8.0f, 4.0f, 4.0f, RAYWHITE);

        DrawCube(
            Vector3{ satellitePos.x - 10.0f, satellitePos.y, satellitePos.z },
            12.0f,
            0.4f,
            5.0f,
            panelColor
        );

        DrawCube(
            Vector3{ satellitePos.x + 10.0f, satellitePos.y, satellitePos.z },
            12.0f,
            0.4f,
            5.0f,
            panelColor
        );

        DrawCubeWires(
            Vector3{ satellitePos.x - 10.0f, satellitePos.y, satellitePos.z },
            12.0f,
            0.4f,
            5.0f,
            RAYWHITE
        );

        DrawCubeWires(
            Vector3{ satellitePos.x + 10.0f, satellitePos.y, satellitePos.z },
            12.0f,
            0.4f,
            5.0f,
            RAYWHITE
        );
    }

    void PrototypeMeshRenderer::render(
        const TransformComponent& transform,
        const RenderableComponent& renderable
    ) const
    {
        Matrix rotationMatrix = QuaternionToMatrix(transform.rotation);
        float16 rotationFloats = MatrixToFloatV(rotationMatrix);

        rlPushMatrix();

        rlTranslatef(transform.position.x, transform.position.y, transform.position.z);
        rlMultMatrixf(rotationFloats.v);
        rlScalef(transform.scale.x, transform.scale.y, transform.scale.z);

        switch (renderable.type)
        {
        case RenderableType::Ship:
            drawShip(renderable.color);
            break;

        case RenderableType::Satellite:
            drawSatellite(renderable.color);
            break;

        default:
            break;
        }

        rlPopMatrix();
    }
}