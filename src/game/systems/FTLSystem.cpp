#include "systems/FTLSystem.h"

#include "components/ShipFlightComponent.h"
#include "components/TransformComponent.h"

#include <raylib.h>
#include <raymath.h>
#include <cmath>
namespace SpaceSim
{
    static Vector3 ToFloatVector(DVec3 value)
    {
        return Vector3{
            static_cast<float>(value.x),
            static_cast<float>(value.y),
            static_cast<float>(value.z)
        };
    }

    static Quaternion GetRotationTowardDirection(Vector3 direction)
    {
        direction = Vector3Normalize(direction);

        // The ship model's forward direction is +Z.
        return QuaternionFromVector3ToVector3(
            Vector3{ 0.0f, 0.0f, 1.0f },
            direction
        );
    }
    static DVec3 NormalizeSafe(DVec3 value)
    {
        const double length = Length(value);

        if (length <= 0.000001)
        {
            return DVec3{ 0.0, 0.0, 0.0 };
        }

        return DVec3{
            value.x / length,
            value.y / length,
            value.z / length
        };
    }

    static void ResetPlayerLocalMotion(GameWorld& world, Vector3 localPosition)
    {
        auto& transform = world.registry.get<TransformComponent>(world.playerShip);
        auto& flight = world.registry.get<ShipFlightComponent>(world.playerShip);

        transform.position = localPosition;

        flight.velocity = Vector3{ 0.0f, 0.0f, 0.0f };
        flight.angularVelocity = Vector3{ 0.0f, 0.0f, 0.0f };
        flight.throttle = 0.0f;
    }

    void FTLSystem::update(GameWorld& world, float dt)
    {
        if (world.travelMode == TravelMode::NormalFlight)
        {
            if (IsKeyPressed(KEY_TAB))
            {
                selectNextJumpTarget(world);
            }

            if (IsKeyPressed(KEY_J))
            {
                beginFTLTravel(world);
            }

            return;
        }

        if (world.travelMode == TravelMode::FTLTravel)
        {
            if (IsKeyPressed(KEY_C))
            {
                cancelFTLTravel(world);
                return;
            }

            updateFTLTravel(world, dt);
        }
    }

    void FTLSystem::selectNextJumpTarget(GameWorld& world)
    {
        if (world.starSystem.objects.empty())
        {
            world.selectedJumpTarget = -1;
            return;
        }

        int startIndex = world.selectedJumpTarget;

        for (int i = 0; i < static_cast<int>(world.starSystem.objects.size()); ++i)
        {
            startIndex = (startIndex + 1) % static_cast<int>(world.starSystem.objects.size());

            if (world.starSystem.objects[startIndex].isJumpTarget)
            {
                world.selectedJumpTarget = startIndex;
                return;
            }
        }

        world.selectedJumpTarget = -1;
    }

    void FTLSystem::beginFTLTravel(GameWorld& world)
    {
        if (world.selectedJumpTarget < 0 ||
            world.selectedJumpTarget >= static_cast<int>(world.starSystem.objects.size()))
        {
            return;
        }

        const GlobalObject& target = world.starSystem.objects[world.selectedJumpTarget];

        if (!target.isJumpTarget)
        {
            return;
        }

        world.travelMode = TravelMode::FTLTravel;

        world.ftlTravel.targetIndex = world.selectedJumpTarget;
        world.ftlTravel.start = world.globalPlayerPosition;
        world.ftlTravel.destination = target.position;
        world.ftlTravel.chargeTimer = 0.0f;

        world.activePoi = -1;

        ResetPlayerLocalMotion(world, Vector3{ 0.0f, 0.0f, 0.0f });
    }

    void FTLSystem::updateFTLTravel(GameWorld& world, float dt)
    {
        auto& transform = world.registry.get<TransformComponent>(world.playerShip);

        const DVec3 toDestinationGlobal =
            world.ftlTravel.destination - world.globalPlayerPosition;

        const double distanceToDestination = Length(toDestinationGlobal);

        if (distanceToDestination <= world.ftlTravel.arrivalDistance)
        {
            arriveFromFTLTravel(world);
            return;
        }

        const DVec3 travelDirectionGlobal = NormalizeSafe(toDestinationGlobal);
        Vector3 travelDirection = ToFloatVector(travelDirectionGlobal);

        Quaternion targetRotation = GetRotationTowardDirection(travelDirection);

        float alignAmount = Clamp(world.ftlTravel.alignSpeed * dt, 0.0f, 1.0f);
        transform.rotation = QuaternionSlerp(
            transform.rotation,
            targetRotation,
            alignAmount
        );

        world.ftlTravel.chargeTimer += dt;

        if (world.ftlTravel.chargeTimer < world.ftlTravel.chargeTime)
        {
            return;
        }

        const double travelDistance = world.ftlTravel.speed * static_cast<double>(dt);

        if (travelDistance >= distanceToDestination)
        {
            world.globalPlayerPosition = world.ftlTravel.destination;
            arriveFromFTLTravel(world);
            return;
        }

        world.globalPlayerPosition =
            world.globalPlayerPosition + travelDirectionGlobal * travelDistance;
    }

    void FTLSystem::arriveFromFTLTravel(GameWorld& world)
    {
        const int targetIndex = world.ftlTravel.targetIndex;

        if (targetIndex < 0 ||
            targetIndex >= static_cast<int>(world.starSystem.objects.size()))
        {
            world.travelMode = TravelMode::NormalFlight;
            world.ftlTravel = FTLTravelState{};
            return;
        }

        const GlobalObject& target = world.starSystem.objects[targetIndex];

        world.globalPlayerPosition = target.position;
        world.activeBubbleOrigin = target.position;
        world.activePoi = targetIndex;

        ResetPlayerLocalMotion(world, Vector3{ 0.0f, 0.0f, -300.0f });

        world.travelMode = TravelMode::NormalFlight;
        world.ftlTravel = FTLTravelState{};
    }

    void FTLSystem::cancelFTLTravel(GameWorld& world)
    {
        world.activeBubbleOrigin = world.globalPlayerPosition;
        world.activePoi = -1;

        ResetPlayerLocalMotion(world, Vector3{ 0.0f, 0.0f, 0.0f });

        world.travelMode = TravelMode::NormalFlight;
        world.ftlTravel = FTLTravelState{};
    }
}