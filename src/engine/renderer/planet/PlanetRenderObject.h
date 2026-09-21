#pragma once

#include "renderer/planet/PlanetMaterial.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>


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

        // Explicit ocean eligibility and physical scale,
        // independent of atmosphere.
        bool hasOcean = false;

        float radiusKm = 0.0f;

        float oceanGravityMetersPerSecondSquared =
            9.81f;


        // Stable near-surface frame data extracted from the
        // double-precision gameplay coordinates.
        //
        // The local ocean pass uses these instead of trying to
        // recover an 8 km camera-to-surface distance by
        // subtracting two ~63,000 render-unit float values.
        bool surfaceFrameValid =
            false;


        float surfaceAltitudeKm =
            0.0f;


        glm::vec3 surfaceAnchorNormalWorld
        {
            0.0f,
            1.0f,
            0.0f
        };


        // Camera-relative position of the sea-level point directly
        // underneath the camera.
        //
        // At 8 km altitude this is only ~8 km worth of render
        // units, rather than planet-center minus planet-radius.
        glm::vec3 surfaceAnchorRelativeWorld
        {
            0.0f
        };
    };
}