#pragma once

#include "renderer/planet/PlanetMaterial.h"

#include <glm/mat4x4.hpp>


namespace SpaceSim
{
    class GpuMesh;


    struct PlanetRenderObject
    {
        const GpuMesh* mesh =
            nullptr;


        glm::mat4 modelMatrix
        {
            1.0f
        };


        PlanetMaterial material;
        bool useAdaptiveTerrain = false;
        // Explicit ocean eligibility and physical scale, independent of atmosphere.
        bool hasOcean = false;
        float radiusKm = 0.0f;
        float oceanGravityMetersPerSecondSquared = 9.81f;
    };
}