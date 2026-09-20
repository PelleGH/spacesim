#include "game/systems/HyperdriveSystem.h"

#include "game/ecs/components/GlobalPositionComponent.h"
#include "game/ecs/components/HyperdriveComponents.h"
#include "game/ecs/components/LightingComponents.h"
#include "game/ecs/components/LocalBubbleTransientComponent.h"
#include "game/ecs/components/PlanetComponents.h"
#include "game/ecs/components/PlayerControlledComponent.h"
#include "game/ecs/components/ShipControlComponent.h"
#include "game/ecs/components/ShipMovementComponent.h"
#include "game/ecs/components/TransformComponent.h"
#include "game/world/SpaceScale.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace SpaceSim
{
    namespace
    {
        glm::quat orientationFromForwardUp(const glm::vec3& forward, const glm::vec3& requestedUp)
        {
            const glm::vec3 f = glm::normalize(forward);

            glm::vec3 up = requestedUp;
            if (glm::length(up) < 0.0001f)
            {
                up = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            up = glm::normalize(up);
            if (std::abs(glm::dot(f, up)) > 0.98f)
            {
                up = glm::vec3(1.0f, 0.0f, 0.0f);
            }

            const glm::vec3 right = glm::normalize(glm::cross(f, up));
            const glm::vec3 correctedUp = glm::normalize(glm::cross(right, f));

            glm::mat3 basis(1.0f);
            basis[0] = right;
            basis[1] = correctedUp;
            basis[2] = -f;
            return glm::normalize(glm::quat_cast(basis));
        }

        float quaternionAngularDistance(glm::quat a, glm::quat b)
        {
            a = glm::normalize(a);
            b = glm::normalize(b);
            const float d = glm::clamp(std::abs(glm::dot(a, b)), 0.0f, 1.0f);
            return 2.0f * std::acos(d);
        }

        std::vector<entt::entity> collectJumpPoints(GameWorld& world)
        {
            std::vector<entt::entity> targets;
            const auto view = world.registry.view<JumpPointComponent, GlobalPositionComponent>();
            for (const entt::entity entity : view)
            {
                targets.push_back(entity);
            }

            std::sort(targets.begin(), targets.end(), [](entt::entity a, entt::entity b)
            {
                return entt::to_integral(a) < entt::to_integral(b);
            });

            return targets;
        }

        void printSelectedTarget(const GameWorld& world, entt::entity target)
        {
            if (target == entt::null ||
                !world.registry.valid(target) ||
                !world.registry.all_of<JumpPointComponent>(target))
            {
                std::cout << "Hyperdrive target: none\n";
                return;
            }

            const auto& jumpPoint = world.registry.get<JumpPointComponent>(target);
            std::cout << "Hyperdrive target: " << jumpPoint.displayName;

            if (world.registry.all_of<PlanetComponent>(target))
            {
                const auto& planet = world.registry.get<PlanetComponent>(target);
                const double altitudeMeters = std::max(
                    0.0,
                    jumpPoint.arrivalDistanceMeters - planet.radiusMeters);
                std::cout << " | arrival altitude "
                          << altitudeMeters / SpaceScale::MetersPerKilometer
                          << " km";
            }

            std::cout << '\n';
        }

        glm::dvec3 safeNormalized(const glm::dvec3& value, const glm::dvec3& fallback)
        {
            const double len = glm::length(value);
            return len > 0.000001 ? value / len : fallback;
        }

        glm::dvec3 chooseArrivalOutward(
            const GameWorld& world,
            entt::entity target,
            const glm::dvec3& shipGlobalMeters,
            const glm::dvec3& targetGlobalMeters)
        {
            glm::dvec3 currentOutward = safeNormalized(
                shipGlobalMeters - targetGlobalMeters,
                glm::dvec3(0.0, 0.0, 1.0));

            // Non-planet targets keep the old approach direction. Planet/moon
            // arrivals are constrained to the daylight hemisphere so a jump
            // never intentionally drops the player onto the dark side.
            if (!world.registry.all_of<PlanetComponent>(target) ||
                world.primaryStar == entt::null ||
                !world.registry.valid(world.primaryStar) ||
                !world.registry.all_of<PrimaryStarComponent>(world.primaryStar))
            {
                return currentOutward;
            }

            const auto& star = world.registry.get<PrimaryStarComponent>(world.primaryStar);
            const glm::dvec3 sunDirection = safeNormalized(
                glm::dvec3(star.direction),
                glm::dvec3(0.0, 1.0, 0.0));

            // Require the sun to be comfortably above the arrival horizon,
            // while selecting the closest daylight-side radial direction to the
            // ship's current approach. This avoids routing straight through the
            // planet merely to reach the exact sub-solar point.
            constexpr double minimumIlluminationDot = 0.25;
            const double currentIllumination = glm::dot(currentOutward, sunDirection);
            if (currentIllumination >= minimumIlluminationDot)
            {
                return currentOutward;
            }

            glm::dvec3 tangent =
                currentOutward - sunDirection * currentIllumination;
            if (glm::length(tangent) < 0.000001)
            {
                const glm::dvec3 helper = std::abs(sunDirection.y) < 0.9
                    ? glm::dvec3(0.0, 1.0, 0.0)
                    : glm::dvec3(1.0, 0.0, 0.0);
                tangent = glm::cross(sunDirection, helper);
            }
            tangent = glm::normalize(tangent);

            const double tangentWeight =
                std::sqrt(1.0 - minimumIlluminationDot * minimumIlluminationDot);
            return glm::normalize(
                sunDirection * minimumIlluminationDot + tangent * tangentWeight);
        }

        void unloadLocalTransientContent(GameWorld& world, entt::entity ship)
        {
            std::vector<entt::entity> transientEntities;
            const auto transientView = world.registry.view<LocalBubbleTransientComponent>();
            for (const entt::entity entity : transientView)
            {
                if (entity != ship && entity != world.activeCamera)
                {
                    transientEntities.push_back(entity);
                }
            }

            for (const entt::entity entity : transientEntities)
            {
                if (world.registry.valid(entity))
                {
                    world.registry.destroy(entity);
                }
            }
        }

        void rebaseBubbleToShip(GameWorld& world, TransformComponent& shipTransform)
        {
            const glm::dvec3 rebaseOffset = shipTransform.positionMeters;
            if (glm::length(rebaseOffset) <= 0.000001)
            {
                return;
            }

            // Preserve every local entity's global position while making the
            // player's current position the new local origin.
            auto transformView = world.registry.view<TransformComponent>();
            for (const entt::entity entity : transformView)
            {
                auto& transform = transformView.get<TransformComponent>(entity);
                transform.positionMeters -= rebaseOffset;
            }

            world.localBubbleOriginMeters += rebaseOffset;
        }
    }

    void HyperdriveSystem::update(GameWorld& world)
    {
        const auto view = world.registry.view<
            PlayerControlledComponent,
            HyperdriveComponent,
            HyperdriveControlComponent>();

        for (const entt::entity ship : view)
        {
            auto& hyperdrive = view.get<HyperdriveComponent>(ship);
            auto& input = view.get<HyperdriveControlComponent>(ship);

            if (input.cycleTargetRequested)
            {
                if (hyperdrive.state == HyperdriveState::Idle)
                {
                    cycleTarget(world, ship);
                }
                else
                {
                    std::cout << "Hyperdrive: target selection locked during an active jump.\n";
                }
            }

            if (input.engageRequested)
            {
                if (hyperdrive.state == HyperdriveState::Idle)
                {
                    beginJump(world, ship);
                }
                else
                {
                    std::cout << "Hyperdrive already active.\n";
                }
            }

            input.cycleTargetRequested = false;
            input.engageRequested = false;
        }
    }

    void HyperdriveSystem::fixedUpdate(GameWorld& world, float dt)
    {
        const auto view = world.registry.view<
            PlayerControlledComponent,
            HyperdriveComponent,
            TransformComponent,
            ShipMovementComponent>();

        for (const entt::entity ship : view)
        {
            auto& hyperdrive = view.get<HyperdriveComponent>(ship);
            if (hyperdrive.state != HyperdriveState::Idle)
            {
                updateActiveJump(world, ship, dt);
            }
        }
    }

    void HyperdriveSystem::cycleTarget(GameWorld& world, entt::entity ship)
    {
        auto& hyperdrive = world.registry.get<HyperdriveComponent>(ship);
        const std::vector<entt::entity> targets = collectJumpPoints(world);

        if (targets.empty())
        {
            hyperdrive.selectedTarget = entt::null;
            std::cout << "Hyperdrive: no jump points available.\n";
            return;
        }

        auto current = std::find(targets.begin(), targets.end(), hyperdrive.selectedTarget);
        if (current == targets.end())
        {
            hyperdrive.selectedTarget = targets.front();
        }
        else
        {
            ++current;
            hyperdrive.selectedTarget = (current == targets.end()) ? targets.front() : *current;
        }

        printSelectedTarget(world, hyperdrive.selectedTarget);
    }

    bool HyperdriveSystem::beginJump(GameWorld& world, entt::entity ship)
    {
        auto& hyperdrive = world.registry.get<HyperdriveComponent>(ship);

        if (hyperdrive.selectedTarget == entt::null)
        {
            std::cout << "Hyperdrive: select a jump point with Tab first.\n";
            return false;
        }

        const entt::entity target = hyperdrive.selectedTarget;
        if (!world.registry.valid(target) ||
            !world.registry.all_of<JumpPointComponent, GlobalPositionComponent>(target) ||
            !world.registry.all_of<TransformComponent, ShipMovementComponent>(ship))
        {
            std::cout << "Hyperdrive: selected jump point is no longer valid.\n";
            hyperdrive.selectedTarget = entt::null;
            return false;
        }

        auto& shipTransform = world.registry.get<TransformComponent>(ship);
        auto& movement = world.registry.get<ShipMovementComponent>(ship);
        const auto& targetPosition = world.registry.get<GlobalPositionComponent>(target);
        const auto& jumpPoint = world.registry.get<JumpPointComponent>(target);

        // Rebase before travel so the ship can remain at local zero while the
        // gameplay bubble itself advances through double-precision global space.
        rebaseBubbleToShip(world, shipTransform);
        const glm::dvec3 shipGlobalMeters = world.localBubbleOriginMeters;

        const glm::dvec3 arrivalOutward = chooseArrivalOutward(
            world,
            target,
            shipGlobalMeters,
            targetPosition.positionMeters);

        hyperdrive.activeTarget = target;
        hyperdrive.arrivalGlobalMeters =
            targetPosition.positionMeters + arrivalOutward * jumpPoint.arrivalDistanceMeters;
        hyperdrive.remainingDistanceMeters = glm::length(
            hyperdrive.arrivalGlobalMeters - shipGlobalMeters);
        hyperdrive.travelSpeedMetersPerSecond = 0.0;
        hyperdrive.state = HyperdriveState::Aligning;

        movement.velocityMetersPerSecond = glm::dvec3(0.0);
        movement.angularVelocityLocalRadiansPerSecond = glm::vec3(0.0f);

        if (world.registry.all_of<ShipControlComponent>(ship))
        {
            auto& control = world.registry.get<ShipControlComponent>(ship);
            control.linearInput = glm::vec3(0.0f);
            control.angularInput = glm::vec3(0.0f);
            control.mouseLookRate = glm::vec2(0.0f);
            control.boost = false;
        }

        unloadLocalTransientContent(world, ship);

        std::cout << "Hyperdrive aligning: " << jumpPoint.displayName
                  << " | "
                  << hyperdrive.remainingDistanceMeters / SpaceScale::MetersPerKilometer
                  << " km to arrival\n";
        return true;
    }

    void HyperdriveSystem::updateActiveJump(GameWorld& world, entt::entity ship, float dt)
    {
        auto& hyperdrive = world.registry.get<HyperdriveComponent>(ship);
        auto& shipTransform = world.registry.get<TransformComponent>(ship);
        auto& movement = world.registry.get<ShipMovementComponent>(ship);

        const entt::entity target = hyperdrive.activeTarget;
        if (target == entt::null ||
            !world.registry.valid(target) ||
            !world.registry.all_of<JumpPointComponent, GlobalPositionComponent>(target))
        {
            std::cout << "Hyperdrive aborted: destination became invalid.\n";
            hyperdrive.state = HyperdriveState::Idle;
            hyperdrive.activeTarget = entt::null;
            hyperdrive.travelSpeedMetersPerSecond = 0.0;
            hyperdrive.remainingDistanceMeters = 0.0;
            return;
        }

        const glm::dvec3 shipGlobalMeters =
            world.localToGlobalMeters(shipTransform.positionMeters);
        const glm::dvec3 toArrivalMeters =
            hyperdrive.arrivalGlobalMeters - shipGlobalMeters;
        const double remainingMeters = glm::length(toArrivalMeters);
        hyperdrive.remainingDistanceMeters = remainingMeters;

        if (remainingMeters <= 1.0)
        {
            finishJump(world, ship);
            return;
        }

        const glm::dvec3 travelDirectionDouble = toArrivalMeters / remainingMeters;
        const glm::vec3 travelDirection = glm::normalize(glm::vec3(travelDirectionDouble));
        const glm::vec3 currentUp = glm::normalize(
            shipTransform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat desiredRotation = orientationFromForwardUp(travelDirection, currentUp);

        if (glm::dot(shipTransform.rotation, desiredRotation) < 0.0f)
        {
            desiredRotation = -desiredRotation;
        }

        if (hyperdrive.state == HyperdriveState::Aligning)
        {
            const float angle = quaternionAngularDistance(
                shipTransform.rotation,
                desiredRotation);

            if (angle <= hyperdrive.alignmentToleranceRadians)
            {
                shipTransform.rotation = desiredRotation;
                movement.angularVelocityLocalRadiansPerSecond = glm::vec3(0.0f);
                hyperdrive.state = HyperdriveState::Traveling;

                const auto& jumpPoint = world.registry.get<JumpPointComponent>(target);
                std::cout << "Hyperdrive engaged: " << jumpPoint.displayName << '\n';
                return;
            }

            const float maxStep = hyperdrive.alignmentTurnRateRadiansPerSecond * dt;
            const float blend = glm::clamp(maxStep / std::max(angle, 0.000001f), 0.0f, 1.0f);
            shipTransform.rotation = glm::normalize(glm::slerp(
                shipTransform.rotation,
                desiredRotation,
                blend));
            return;
        }

        // Hyperdrive is global-space translation. The ship remains at local
        // origin while the bubble origin advances in double precision.
        shipTransform.rotation = desiredRotation;
        shipTransform.positionMeters = glm::dvec3(0.0);
        movement.velocityMetersPerSecond = glm::dvec3(0.0);
        movement.angularVelocityLocalRadiansPerSecond = glm::vec3(0.0f);

        const double acceleration =
            hyperdrive.travelAccelerationMetersPerSecondSquared;
        const double stoppingSpeed = std::sqrt(std::max(
            0.0,
            2.0 * acceleration * remainingMeters));
        // Accelerate toward cruise speed, but never exceed the speed from
        // which the ship can still brake over the remaining distance. This
        // gives a simple Elite/Star-Citizen-like accelerate/cruise/decelerate
        // profile without a scripted travel timer.
        const double acceleratedSpeed = std::min(
            hyperdrive.maximumTravelSpeedMetersPerSecond,
            hyperdrive.travelSpeedMetersPerSecond +
                acceleration * static_cast<double>(dt));
        hyperdrive.travelSpeedMetersPerSecond = std::min(
            acceleratedSpeed,
            stoppingSpeed);

        const double travelStepMeters = std::min(
            remainingMeters,
            hyperdrive.travelSpeedMetersPerSecond * static_cast<double>(dt));

        world.localBubbleOriginMeters +=
            travelDirectionDouble * travelStepMeters;

        hyperdrive.remainingDistanceMeters = std::max(
            0.0,
            remainingMeters - travelStepMeters);

        if (travelStepMeters >= remainingMeters - 0.001)
        {
            finishJump(world, ship);
        }
    }

    void HyperdriveSystem::finishJump(GameWorld& world, entt::entity ship)
    {
        auto& hyperdrive = world.registry.get<HyperdriveComponent>(ship);
        auto& shipTransform = world.registry.get<TransformComponent>(ship);
        auto& movement = world.registry.get<ShipMovementComponent>(ship);

        world.localBubbleOriginMeters = hyperdrive.arrivalGlobalMeters;
        shipTransform.positionMeters = glm::dvec3(0.0);
        movement.velocityMetersPerSecond = glm::dvec3(0.0);
        movement.angularVelocityLocalRadiansPerSecond = glm::vec3(0.0f);

        const entt::entity target = hyperdrive.activeTarget;
        std::string targetName = "destination";
        if (target != entt::null &&
            world.registry.valid(target) &&
            world.registry.all_of<JumpPointComponent>(target))
        {
            const auto& jumpPoint = world.registry.get<JumpPointComponent>(target);
            targetName = jumpPoint.displayName;

            std::cout << "Hyperdrive exit: " << targetName;
            if (world.registry.all_of<PlanetComponent>(target))
            {
                const auto& planet = world.registry.get<PlanetComponent>(target);
                const double altitudeMeters = std::max(
                    0.0,
                    jumpPoint.arrivalDistanceMeters - planet.radiusMeters);
                std::cout << " | "
                          << altitudeMeters / SpaceScale::MetersPerKilometer
                          << " km above surface";
            }
            std::cout << '\n';
        }

        hyperdrive.state = HyperdriveState::Idle;
        hyperdrive.activeTarget = entt::null;
        hyperdrive.travelSpeedMetersPerSecond = 0.0;
        hyperdrive.remainingDistanceMeters = 0.0;
    }

    std::string HyperdriveSystem::selectedTargetName(const GameWorld& world) const
    {
        if (world.playerShip == entt::null ||
            !world.registry.valid(world.playerShip) ||
            !world.registry.all_of<HyperdriveComponent>(world.playerShip))
        {
            return "None";
        }

        const auto& hyperdrive = world.registry.get<HyperdriveComponent>(world.playerShip);
        if (hyperdrive.selectedTarget == entt::null ||
            !world.registry.valid(hyperdrive.selectedTarget) ||
            !world.registry.all_of<JumpPointComponent>(hyperdrive.selectedTarget))
        {
            return "None";
        }

        return world.registry.get<JumpPointComponent>(hyperdrive.selectedTarget).displayName;
    }

    std::string HyperdriveSystem::stateName(const GameWorld& world) const
    {
        if (world.playerShip == entt::null ||
            !world.registry.valid(world.playerShip) ||
            !world.registry.all_of<HyperdriveComponent>(world.playerShip))
        {
            return "Idle";
        }

        switch (world.registry.get<HyperdriveComponent>(world.playerShip).state)
        {
        case HyperdriveState::Aligning:
            return "Aligning";
        case HyperdriveState::Traveling:
            return "Traveling";
        case HyperdriveState::Idle:
        default:
            return "Idle";
        }
    }
}
