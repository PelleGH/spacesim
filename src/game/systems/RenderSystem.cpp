#include "systems/RenderSystem.h"

#include "components/TransformComponent.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string>

namespace SpaceSim
{
    static Color GetPlanetColor(const std::string& name)
    {
        if (name == "Aster")
        {
            return BLUE;
        }

        if (name == "Boreal")
        {
            return GREEN;
        }

        if (name == "Cyra")
        {
            return PURPLE;
        }

        return ORANGE;
    }
    static int FindParentPlanetIndexForRelay(const GameWorld& world, int relayIndex)
    {
        if (relayIndex < 0 ||
            relayIndex >= static_cast<int>(world.starSystem.objects.size()))
        {
            return -1;
        }

        const auto& relay = world.starSystem.objects[relayIndex];

        if (relay.name == "Aster Relay")
        {
            return 1;
        }

        if (relay.name == "Boreal Relay")
        {
            return 2;
        }

        if (relay.name == "Cyra Relay")
        {
            return 3;
        }

        return -1;
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

    static void DrawSatellite(Vector3 satellitePos, Color panelColor)
    {
        DrawCube(satellitePos, 8.0f, 4.0f, 4.0f, LIGHTGRAY);
        DrawCubeWires(satellitePos, 8.0f, 4.0f, 4.0f, RAYWHITE);

        DrawCube(
            Vector3{ satellitePos.x - 10.0f, satellitePos.y, satellitePos.z },
            12.0f,
            0.5f,
            5.0f,
            panelColor
        );

        DrawCube(
            Vector3{ satellitePos.x + 10.0f, satellitePos.y, satellitePos.z },
            12.0f,
            0.5f,
            5.0f,
            panelColor
        );

        DrawSphere(
            Vector3{ satellitePos.x, satellitePos.y + 4.0f, satellitePos.z },
            1.5f,
            RED
        );
    }
    static void DrawActiveParentPlanetSky3D(const GameWorld& world)
    {
        int parentPlanetIndex = FindParentPlanetIndexForRelay(world, world.activePoi);

        if (parentPlanetIndex < 0 ||
            parentPlanetIndex >= static_cast<int>(world.starSystem.objects.size()))
        {
            return;
        }

        const auto& planet = world.starSystem.objects[parentPlanetIndex];

        DVec3 relative = planet.position - world.globalPlayerPosition;
        double globalDistance = Length(relative);

        if (globalDistance < 1.0)
        {
            return;
        }

        Vector3 direction = ToRenderDirection(relative);

        constexpr float skyDistance = 850.0f;

        Vector3 skyPos = Vector3Scale(direction, skyDistance);

        float radius = static_cast<float>(planet.visualRadius / globalDistance * 850.0);
        radius = Clamp(radius, 180.0f, 360.0f);

        DrawSphere(skyPos, radius, GetPlanetColor(planet.name));
        DrawSphereWires(skyPos, radius, 32, 32, RAYWHITE);
    }
    static void DrawDistantStarSystem3D(const GameWorld& world)
    {
        int localParentPlanetIndex = FindParentPlanetIndexForRelay(world, world.activePoi);

        constexpr float skyDistance = 900.0f;

        for (const auto& object : world.starSystem.objects)
        {
            int objectIndex = static_cast<int>(&object - world.starSystem.objects.data());

            // The current relay's parent planet is drawn as a real local 3D object.
            if (objectIndex == localParentPlanetIndex)
            {
                continue;
            }

            if (object.type == GlobalObjectType::Satellite)
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

            Color color = WHITE;
            float radius = 40.0f;

            switch (object.type)
            {
            case GlobalObjectType::Sun:
                color = YELLOW;
                radius = 110.0f;
                break;

            case GlobalObjectType::Planet:
                color = GetPlanetColor(object.name);

                // Prototype apparent size.
                radius = static_cast<float>(object.visualRadius / globalDistance * 3500.0);
                radius = Clamp(radius, 25.0f, 70.0f);
                break;

            case GlobalObjectType::Satellite:
                break;
            }

            DrawSphere(skyPos, radius, color);
            DrawSphereWires(skyPos, radius, 24, 24, RAYWHITE);
        }
    }

    static void DrawLocalBubble(const GameWorld& world)
    {
        // Local asteroid field near the current active bubble.
        int sceneSeed = world.activePoi < 0 ? 0 : world.activePoi * 101;

        for (int i = 0; i < 24; ++i)
        {
            float x = static_cast<float>(((i * 37 + sceneSeed) % 160) - 80);
            float y = static_cast<float>(((i * 53 + sceneSeed * 2) % 80) - 40);
            float z = 80.0f + static_cast<float>((i * 22 + sceneSeed) % 700);

            float size = 3.0f + static_cast<float>(((i * 11 + sceneSeed) % 8));

            DrawSphere(Vector3{ x, y, z }, size, GRAY);
            DrawSphereWires(Vector3{ x, y, z }, size, 8, 8, DARKGRAY);
        }

        // Active local POI.
        Color relayColor = BLUE;

        if (world.activePoi >= 0 &&
            world.activePoi < static_cast<int>(world.starSystem.objects.size()))
        {
            const auto& activePoi = world.starSystem.objects[world.activePoi];

            if (activePoi.name == "Aster Relay")
            {
                relayColor = BLUE;
            }
            else if (activePoi.name == "Boreal Relay")
            {
                relayColor = GREEN;
            }
            else if (activePoi.name == "Cyra Relay")
            {
                relayColor = PURPLE;
            }
        }

        DrawSatellite(Vector3{ 0.0f, 0.0f, 0.0f }, relayColor);
    }
    void RenderSystem::renderSky(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        // Camera rotation only. Position is locked to origin so the sky does not
        // slide around when the player moves locally.
        Vector3 cameraForward = Vector3Normalize(Vector3Subtract(
            world.camera.target,
            world.camera.position
        ));

        Camera3D skyCamera{};
        skyCamera.position = Vector3{ 0.0f, 0.0f, 0.0f };
        skyCamera.target = cameraForward;
        skyCamera.up = world.camera.up;
        skyCamera.fovy = world.camera.fovy;
        skyCamera.projection = world.camera.projection;

        BeginMode3D(skyCamera);

        // The sky is its own layer. It should never affect the depth buffer used by
        // local gameplay objects.
        rlDisableDepthTest();
        rlDisableDepthMask();

        // Simple temporary starfield.
        for (int i = 0; i < 300; ++i)
        {
            float x = static_cast<float>((i * 97) % 1000 - 500);
            float y = static_cast<float>((i * 193) % 1000 - 500);
            float z = static_cast<float>((i * 389) % 1000 - 500);

            Vector3 dir = Vector3Normalize(Vector3{ x, y, z });
            DrawSphere(Vector3Scale(dir, 850.0f), 0.6f, RAYWHITE);
        }

        DrawDistantStarSystem3D(world);

        // Draw the current relay's parent planet last in the sky layer.
        // This is the dominant nearby celestial backdrop.
        DrawActiveParentPlanetSky3D(world);

        rlEnableDepthMask();
        rlEnableDepthTest();

        EndMode3D();
    }

    void RenderSystem::renderWorld(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        DrawLocalBubble(world);

        auto view = world.registry.view<TransformComponent>();

        for (auto entity : view)
        {
            (void)entity;

            const auto& transform = view.get<TransformComponent>(entity);

            Matrix rotationMatrix = QuaternionToMatrix(transform.rotation);
            float16 rotationFloats = MatrixToFloatV(rotationMatrix);

            rlPushMatrix();

            rlTranslatef(transform.position.x, transform.position.y, transform.position.z);
            rlMultMatrixf(rotationFloats.v);
            rlScalef(transform.scale.x, transform.scale.y, transform.scale.z);

            DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, 0.4f, 2.0f, SKYBLUE);
            DrawCubeWires(Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, 0.4f, 2.0f, WHITE);
            DrawCube(Vector3{ 0.0f, 0.0f, 1.25f }, 0.35f, 0.25f, 0.35f, RED);

            rlPopMatrix();
        }
    }
}