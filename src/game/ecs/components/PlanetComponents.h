#pragma once

#include "renderer/atmosphere/AtmosphereParameters.h"
#include "renderer/planet/PlanetMaterial.h"

namespace SpaceSim
{
    struct PlanetComponent
    {
        // Gameplay/world truth is physical. There is intentionally no
        // renderer-specific radiusWorld here anymore.
        double radiusMeters = 6371000.0;
        bool hasOcean = true;
        float oceanGravityMetersPerSecondSquared = 9.81f;
    };

    // Visual parameters are allowed in ECS, but GPU objects are not.
    struct PlanetVisualComponent
    {
        PlanetMaterial material;
    };

    struct AtmosphereComponent
    {
        AtmosphereParameters parameters = makeEarthLikeAtmosphere();
        bool enabled = true;
    };
}
