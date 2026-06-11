#pragma once

#include "world/PlanetVisualGenerator.h"
#include "world/StarSystem.h"

#include <raylib.h>

namespace SpaceSim
{
    struct PlanetTerrainSample
    {
        float height = 0.0f;          // normalized visual height, not meters yet
        Color color = WHITE;
    };

    PlanetTerrainSample SamplePlanetTerrain(
        const GlobalObject& object,
        Vector3 unitDirection
    );

    float SamplePlanetHeight(
        const GlobalObject& object,
        Vector3 unitDirection
    );

    Color SamplePlanetSurfaceColor(
        const GlobalObject& object,
        Vector3 unitDirection
    );

    Vector3 EstimatePlanetNormal(
        const GlobalObject& object,
        Vector3 unitDirection
    );
}