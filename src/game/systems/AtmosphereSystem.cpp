#include "systems/AtmosphereSystem.h"

#include "world/GameWorld.h"

#include <algorithm>

namespace SpaceSim
{
    void AtmosphereSystem::update(GameWorld& world)
    {
        AtmosphereState& atmosphere = world.atmosphere;

        atmosphere.insideAtmosphere = false;
        atmosphere.density = 0.0f;

        if (world.planetTransition.mode == PlanetRenderMode::Distant)
        {
            return;
        }

        const double altitude = world.planetTransition.altitude;

        if (altitude >= atmosphere.atmosphereHeight)
        {
            return;
        }

        atmosphere.insideAtmosphere = true;

        const double usableAtmosphereHeight = std::max(
            1.0,
            atmosphere.atmosphereHeight - world.orbitalCruise.minimumAltitude);

        const double normalizedDepth = std::clamp(
            (atmosphere.atmosphereHeight - altitude) / usableAtmosphereHeight,
            0.0,
            1.0);

        atmosphere.density = static_cast<float>(normalizedDepth * normalizedDepth);
    }
}
