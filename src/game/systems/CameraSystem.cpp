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
        auto& transform = world.registry.get<TransformComponent>(world.playerShip);

        const bool allowFreeLook = world.travelMode == TravelMode::FTLTravel;
        const bool freeLookHeld = allowFreeLook && IsKeyDown(KEY_LEFT_ALT);

        if (freeLookHeld)
        {
            Vector2 mouseDelta = GetMouseDelta();

            m_freeLook.x -= mouseDelta.x * m_freeLookSensitivity;
            m_freeLook.y -= mouseDelta.y * m_freeLookSensitivity;

            m_freeLook.x = Clamp(m_freeLook.x, -m_maxFreeLookYaw, m_maxFreeLookYaw);
            m_freeLook.y = Clamp(m_freeLook.y, -m_maxFreeLookPitch, m_maxFreeLookPitch);
        }
        else
        {
            const float returnAmount = Clamp(m_freeLookReturnSpeed * dt, 0.0f, 1.0f);

            m_freeLook = Vector2Lerp(
                m_freeLook,
                Vector2{ 0.0f, 0.0f },
                returnAmount
            );
        }

        Quaternion freeLookYaw = QuaternionFromAxisAngle(
            Vector3{ 0.0f, 1.0f, 0.0f },
            m_freeLook.x
        );

        Quaternion freeLookPitch = QuaternionFromAxisAngle(
            Vector3{ 1.0f, 0.0f, 0.0f },
            m_freeLook.y
        );

        Quaternion cameraLookRotation = QuaternionMultiply(
            transform.rotation,
            QuaternionMultiply(freeLookYaw, freeLookPitch)
        );

        Vector3 cameraOffset = Vector3RotateByQuaternion(
            Vector3{ 0.0f, 1.0f, -8.0f },
            transform.rotation
        );

        Vector3 lookAhead = Vector3RotateByQuaternion(
            Vector3{ 0.0f, 0.6f, 12.0f },
            cameraLookRotation
        );

        Vector3 cameraUp = Vector3RotateByQuaternion(
            Vector3{ 0.0f, 1.0f, 0.0f },
            cameraLookRotation
        );

        world.camera.position = Vector3Add(transform.position, cameraOffset);
        world.camera.target = Vector3Add(transform.position, lookAhead);
        world.camera.up = cameraUp;
    }
}