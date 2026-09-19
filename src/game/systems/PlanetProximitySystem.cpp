#include "systems/PlanetProximitySystem.h"

#include "world/GameWorld.h"

#include <limits>

namespace SpaceSim
{
    void PlanetProximitySystem::update(GameWorld& world)
    {
        PlanetTransitionState& state = world.planetTransition;

        int closestPlanetIndex = -1;
        double closestCenterDistance = std::numeric_limits<double>::max();

        for (int i = 0; i < static_cast<int>(world.starSystem.objects.size()); ++i)
        {
            const GlobalObject& object = world.starSystem.objects[i];

            if (object.type != GlobalObjectType::Planet)
            {
                continue;
            }

            const double centerDistance =
                Length(object.position - world.globalPlayerPosition);

            if (centerDistance < closestCenterDistance)
            {
                closestCenterDistance = centerDistance;
                closestPlanetIndex = i;
            }
        }

        state.closestPlanetIndex = closestPlanetIndex;

        if (closestPlanetIndex < 0)
        {
            state.mode = PlanetRenderMode::Distant;
            state.distanceToCenter = 0.0;
            state.altitude = 0.0;
            state.orbitalEnterAltitude = 0.0;
            state.orbitalExitAltitude = 0.0;
            return;
        }

        const GlobalObject& planet = world.starSystem.objects[closestPlanetIndex];

        state.distanceToCenter = closestCenterDistance;
        state.altitude = closestCenterDistance - planet.visualRadius;
        state.orbitalEnterAltitude = planet.visualRadius * 8.0;
        state.orbitalExitAltitude = planet.visualRadius * 10.0;

        switch (state.mode)
        {
        case PlanetRenderMode::Distant:
            if (state.altitude <= state.orbitalEnterAltitude)
            {
                state.mode = PlanetRenderMode::Orbital;
            }
            break;

        case PlanetRenderMode::Orbital:
            if (state.altitude <= state.surfaceEnterAltitude)
            {
                state.mode = PlanetRenderMode::Surface;
            }
            else if (state.altitude >= state.orbitalExitAltitude)
            {
                state.mode = PlanetRenderMode::Distant;
            }
            break;

        case PlanetRenderMode::Surface:
            if (state.altitude >= state.surfaceExitAltitude)
            {
                state.mode = PlanetRenderMode::Orbital;
            }
            break;
        }
    }
}
