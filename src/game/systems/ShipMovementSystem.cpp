#include "game/systems/ShipMovementSystem.h"

#include "game/ecs/components/HyperdriveComponents.h"
#include "game/ecs/components/ShipControlComponent.h"
#include "game/ecs/components/ShipMovementComponent.h"
#include "game/ecs/components/TransformComponent.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace SpaceSim
{
    namespace
    {
        float expApproach(float current, float target, float response, float dt)
        {
            const float blend = 1.0f - std::exp(-response * dt);
            return current + (target - current) * blend;
        }

        void dampAxis(float& value, float input, float damping, float dt)
        {
            if (std::abs(input) < 0.001f)
            {
                value *= std::exp(-damping * dt);
            }
        }
    }

    void ShipMovementSystem::fixedUpdate(GameWorld& world, float dt)
    {
        const auto view = world.registry.view<TransformComponent, ShipControlComponent, ShipMovementComponent>();

        for (const entt::entity entity : view)
        {
            if (world.registry.all_of<HyperdriveComponent>(entity) &&
                world.registry.get<HyperdriveComponent>(entity).state != HyperdriveState::Idle)
            {
                // Alignment and system-scale translation are exclusively owned
                // by HyperdriveSystem while a jump is active.
                continue;
            }

            auto& transform = view.get<TransformComponent>(entity);
            const auto& control = view.get<ShipControlComponent>(entity);
            auto& movement = view.get<ShipMovementComponent>(entity);

            const glm::vec3 right = glm::normalize(transform.rotation * glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::vec3 up = glm::normalize(transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::vec3 forward = glm::normalize(transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f));

            const float forwardAcceleration = control.linearInput.z >= 0.0f
                ? movement.forwardAccelerationMetersPerSecondSquared
                : movement.reverseAccelerationMetersPerSecondSquared;
            const float boostMultiplier = control.boost && control.linearInput.z > 0.0f
                ? movement.boostAccelerationMultiplier
                : 1.0f;

            glm::vec3 accelerationWorld(0.0f);
            accelerationWorld += right * (control.linearInput.x * movement.strafeAccelerationMetersPerSecondSquared);
            accelerationWorld += up * (control.linearInput.y * movement.verticalAccelerationMetersPerSecondSquared);
            accelerationWorld += forward * (control.linearInput.z * forwardAcceleration * boostMultiplier);

            const double speedBeforeAcceleration = glm::length(movement.velocityMetersPerSecond);
            glm::dvec3 acceleration = glm::dvec3(accelerationWorld);

            // Normal thrust stops adding speed once the normal envelope has
            // been reached, but releasing boost does not snap a boosted ship
            // back to maxSpeed. Existing excess velocity is allowed to coast
            // and flight assist bleeds it off naturally when thrust is released.
            if (!control.boost &&
                speedBeforeAcceleration >= static_cast<double>(movement.maxSpeedMetersPerSecond) &&
                speedBeforeAcceleration > 0.000001)
            {
                const glm::dvec3 velocityDirection =
                    movement.velocityMetersPerSecond / speedBeforeAcceleration;
                const double acceleratingAlongVelocity = glm::dot(acceleration, velocityDirection);
                if (acceleratingAlongVelocity > 0.0)
                {
                    acceleration -= velocityDirection * acceleratingAlongVelocity;
                }
            }

            movement.velocityMetersPerSecond += acceleration * static_cast<double>(dt);

            if (control.flightAssist)
            {
                glm::vec3 localVelocity = glm::inverse(transform.rotation) * glm::vec3(movement.velocityMetersPerSecond);
                dampAxis(localVelocity.x, control.linearInput.x, movement.linearDampingPerSecond, dt);
                dampAxis(localVelocity.y, control.linearInput.y, movement.linearDampingPerSecond, dt);
                dampAxis(localVelocity.z, control.linearInput.z, movement.linearDampingPerSecond, dt);
                movement.velocityMetersPerSecond = glm::dvec3(transform.rotation * localVelocity);
            }

            const double speed = glm::length(movement.velocityMetersPerSecond);
            if (speed > 0.0)
            {
                if (!control.boost &&
                    speedBeforeAcceleration < static_cast<double>(movement.maxSpeedMetersPerSecond) &&
                    speed > static_cast<double>(movement.maxSpeedMetersPerSecond))
                {
                    movement.velocityMetersPerSecond *= static_cast<double>(movement.maxSpeedMetersPerSecond) / speed;
                }
                else if (speed > static_cast<double>(movement.boostMaxSpeedMetersPerSecond))
                {
                    movement.velocityMetersPerSecond *= static_cast<double>(movement.boostMaxSpeedMetersPerSecond) / speed;
                }
            }

            transform.positionMeters += movement.velocityMetersPerSecond * static_cast<double>(dt);

            // Ship-control semantics are pitch-up / yaw-right / roll-right.
            // With our model facing local -Z, yaw-right and roll-right rotate
            // around the negative local Y/Z axes respectively.
            glm::vec3 angularAcceleration(
                control.angularInput.x * movement.pitchAccelerationRadiansPerSecondSquared,
                -control.angularInput.y * movement.yawAccelerationRadiansPerSecondSquared,
                -control.angularInput.z * movement.rollAccelerationRadiansPerSecondSquared);
            movement.angularVelocityLocalRadiansPerSecond += angularAcceleration * dt;

            if (control.mouseSteering)
            {
                movement.angularVelocityLocalRadiansPerSecond.x = expApproach(
                    movement.angularVelocityLocalRadiansPerSecond.x,
                    control.mouseLookRate.x,
                    movement.mouseTurnResponse,
                    dt);
                movement.angularVelocityLocalRadiansPerSecond.y = expApproach(
                    movement.angularVelocityLocalRadiansPerSecond.y,
                    -control.mouseLookRate.y,
                    movement.mouseTurnResponse,
                    dt);
            }
            else if (control.flightAssist)
            {
                dampAxis(movement.angularVelocityLocalRadiansPerSecond.x, control.angularInput.x, movement.angularDampingPerSecond, dt);
                dampAxis(movement.angularVelocityLocalRadiansPerSecond.y, control.angularInput.y, movement.angularDampingPerSecond, dt);
            }

            if (control.flightAssist)
            {
                dampAxis(movement.angularVelocityLocalRadiansPerSecond.z, control.angularInput.z, movement.angularDampingPerSecond, dt);
            }

            movement.angularVelocityLocalRadiansPerSecond.x = glm::clamp(
                movement.angularVelocityLocalRadiansPerSecond.x,
                -movement.maxPitchRateRadiansPerSecond,
                movement.maxPitchRateRadiansPerSecond);
            movement.angularVelocityLocalRadiansPerSecond.y = glm::clamp(
                movement.angularVelocityLocalRadiansPerSecond.y,
                -movement.maxYawRateRadiansPerSecond,
                movement.maxYawRateRadiansPerSecond);
            movement.angularVelocityLocalRadiansPerSecond.z = glm::clamp(
                movement.angularVelocityLocalRadiansPerSecond.z,
                -movement.maxRollRateRadiansPerSecond,
                movement.maxRollRateRadiansPerSecond);

            const glm::vec3 angularStep = movement.angularVelocityLocalRadiansPerSecond * dt;
            const float angle = glm::length(angularStep);
            if (angle > 0.000001f)
            {
                const glm::quat deltaRotation = glm::angleAxis(angle, angularStep / angle);
                transform.rotation = glm::normalize(transform.rotation * deltaRotation);
            }
        }
    }
}
