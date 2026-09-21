#include "game/systems/ChaseCameraSystem.h"

#include "game/ecs/components/CameraComponent.h"
#include "game/ecs/components/TransformComponent.h"
#include "game/ecs/components/PreviousTransformComponent.h"
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>

#include <cmath>

namespace SpaceSim
{
    namespace
    {
        glm::quat orientationFromForwardUp(const glm::vec3 &forward, const glm::vec3 &up)
        {
            const glm::vec3 f = glm::normalize(forward);
            const glm::vec3 right = glm::normalize(glm::cross(f, up));
            const glm::vec3 correctedUp = glm::normalize(glm::cross(right, f));

            glm::mat3 basis(1.0f);
            basis[0] = right;
            basis[1] = correctedUp;
            basis[2] = -f;
            return glm::normalize(glm::quat_cast(basis));
        }
    }

    void ChaseCameraSystem::update(GameWorld &world, float dt)
    {
        if (world.playerShip == entt::null ||
            world.activeCamera == entt::null ||
            !world.registry.valid(world.playerShip) ||
            !world.registry.valid(world.activeCamera) ||
            !world.registry.all_of<TransformComponent>(world.playerShip) ||
            !world.registry.all_of<TransformComponent, CameraComponent>(world.activeCamera))
        {
            return;
        }

        const TransformComponent shipTransform = interpolatedTransform(world.registry, world.playerShip, world.renderInterpolationAlpha);
        auto &cameraTransform = world.registry.get<TransformComponent>(world.activeCamera);

        const glm::vec3 shipForward = glm::normalize(
            shipTransform.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
        const glm::vec3 shipUp = glm::normalize(
            shipTransform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));

        // Physical camera dimensions for the ~28 m placeholder ship.
        const glm::vec3 localCameraOffsetMeters(0.0f, 7.5f, 42.0f);
        const glm::dvec3 desiredPositionMeters =
            shipTransform.positionMeters +
            glm::dvec3(shipTransform.rotation * localCameraOffsetMeters);

        const glm::dvec3 lookTargetMeters =
            shipTransform.positionMeters +
            glm::dvec3(shipForward * 8.0f + shipUp * 1.5f);

        const glm::quat desiredRotation = orientationFromForwardUp(
            glm::normalize(glm::vec3(lookTargetMeters - desiredPositionMeters)),
            shipUp);

        if (!m_initialized)
        {
            cameraTransform.positionMeters = desiredPositionMeters;
            cameraTransform.rotation = desiredRotation;
            m_initialized = true;
            return;
        }

        const double positionBlend = 1.0 - std::exp(-9.0 * static_cast<double>(dt));
        const float rotationBlend = 1.0f - std::exp(-12.0f * dt);

        cameraTransform.positionMeters +=
            (desiredPositionMeters - cameraTransform.positionMeters) * positionBlend;
        cameraTransform.rotation = glm::normalize(glm::slerp(
            cameraTransform.rotation,
            desiredRotation,
            rotationBlend));
    }
}
