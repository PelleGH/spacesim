#include "systems/CameraSystem.h"

#include "components/TransformComponent.h"
#include <raymath.h>
namespace SpaceSim
{
    void CameraSystem::initialize(GameWorld& world)
    {
        world.camera.position = Vector3{ 0.0f, 4.0f, -10.0f };
        world.camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
        world.camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
        world.camera.fovy = 60.0f;
        world.camera.projection = CAMERA_PERSPECTIVE;
    }

void CameraSystem::update(GameWorld& world, float dt)
{
    (void)dt;

    auto& transform = world.registry.get<TransformComponent>(world.playerShip);

    Vector3 cameraOffset = Vector3RotateByQuaternion(
        Vector3{ 0.0f, 1.0f, -8.0f },
        transform.rotation
    );

    Vector3 lookAhead = Vector3RotateByQuaternion(
        Vector3{ 0.0f, 0.6f, 12.0f },
        transform.rotation
    );

    Vector3 cameraUp = Vector3RotateByQuaternion(
        Vector3{ 0.0f, 1.0f, 0.0f },
        transform.rotation
    );

    world.camera.position = Vector3Add(transform.position, cameraOffset);
    world.camera.target = Vector3Add(transform.position, lookAhead);
    world.camera.up = cameraUp;
}
}