#pragma once

#include "world/StarSystem.h"

#include <raylib.h>

namespace SpaceSim
{
    struct PlanetSurfaceMaps
    {
        Texture2D albedo{};
        Texture2D material{};
        Texture2D normal{};

        int width = 0;
        int height = 0;
    };

    PlanetSurfaceMaps GeneratePlanetSurfaceMaps(
        const GlobalObject& object,
        int width,
        int height
    );

    void UnloadPlanetSurfaceMaps(PlanetSurfaceMaps& maps);
}