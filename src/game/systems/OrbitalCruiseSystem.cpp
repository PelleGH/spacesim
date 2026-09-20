#include "input/GameInput.h"
#include "systems/OrbitalCruiseSystem.h"

#include "components/TransformComponent.h"
#include "world/GameWorld.h"

#include <algorithm>
#include <raylib.h>
#include <raymath.h>

namespace SpaceSim
{
    void OrbitalCruiseSystem::update(GameWorld& world, float dt)
    {
        OrbitalCruiseState& cruise = world.orbitalCruise;

        cruise.active = false;
        cruise.currentSpeed = 0.0;

        if (world.travelMode != TravelMode::NormalFlight)
        {
            return;
        }

        if (world.planetTransition.mode == PlanetRenderMode::Distant)
        {
            return;
        }

        if (!GameInput::keyDown(KEY_LEFT_SHIFT))
        {
            return;
        }

        if (world.playerShip == entt::null ||
            !world.registry.valid(world.playerShip) ||
            !world.registry.all_of<TransformComponent>(world.playerShip))
        {
            return;
        }

        const int planetIndex = world.planetTransition.closestPlanetIndex;

        if (planetIndex < 0 ||
            planetIndex >= static_cast<int>(world.starSystem.objects.size()))
        {
            return;
        }

        const GlobalObject& planet = world.starSystem.objects[planetIndex];
        const auto& transform = world.registry.get<TransformComponent>(world.playerShip);

        const Vector3 localForward = Vector3Normalize(
            Vector3RotateByQuaternion(Vector3{ 0.0f, 0.0f, 1.0f }, transform.rotation));

        const DVec3 globalForward{
            static_cast<double>(localForward.x),
            static_cast<double>(localForward.y),
            static_cast<double>(localForward.z)
        };

        const double slowdownRange = std::max(
            1.0,
            cruise.slowdownAltitude - cruise.minimumAltitude);

        const double normalizedAltitude = std::clamp(
            (world.planetTransition.altitude - cruise.minimumAltitude) / slowdownRange,
            0.0,
            1.0);

        constexpr double minimumSpeedFraction = 0.10;

        const double speedFraction =
            minimumSpeedFraction + normalizedAltitude * (1.0 - minimumSpeedFraction);

        cruise.currentSpeed = cruise.maximumSpeed * speedFraction;

        const double movementDistance = cruise.currentSpeed * static_cast<double>(dt);
        const DVec3 requestedDisplacement = globalForward * movementDistance;
        DVec3 requestedPlayerPosition = world.globalPlayerPosition + requestedDisplacement;

        const DVec3 planetToRequestedPlayer = requestedPlayerPosition - planet.position;
        const double requestedCenterDistance = Length(planetToRequestedPlayer);
        const double minimumCenterDistance = planet.visualRadius + cruise.minimumAltitude;

        if (requestedCenterDistance < minimumCenterDistance)
        {
            DVec3 outwardDirection = Normalize(planetToRequestedPlayer);
            requestedPlayerPosition = planet.position + outwardDirection * minimumCenterDistance;
        }

        const DVec3 correctedDisplacement =
            requestedPlayerPosition - world.globalPlayerPosition;

        world.activeBubbleOrigin = world.activeBubbleOrigin + correctedDisplacement;
        cruise.active = true;
    }
}
