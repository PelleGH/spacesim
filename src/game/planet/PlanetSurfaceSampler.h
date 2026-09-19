#pragma once

#include "world/StarSystem.h"

#include <raylib.h>
#include <string>

namespace SpaceSim
{
    struct PlanetSurfaceSample
    {
        bool isOcean = false;
        float continentValue = 0.0f;
        float terrainHeight = 0.0f;
        Color color = WHITE;
    };

    class PlanetSurfaceSampler
    {
    public:
        // Terrain authoring units are independent of the camera's render scale.
        // The current ocean-world relief is bounded by 41.5 units, i.e. <0.15%
        // of radius. World elevation = terrainHeight * worldRadius / this value.
        static constexpr float TerrainUnitsPerRadius = 28000.0f;

        [[nodiscard]] static int seedFromId(const std::string& planetId);

        [[nodiscard]] static PlanetSurfaceSample sample(
            Vector3 radialDirection,
            int seed,
            PlanetClass planetClass);
    };
}
