#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct PlanetMaterial
    {
        // =========================================================
        // OCEAN
        // =========================================================

        glm::vec3 deepOceanColor{
            0.008f,
            0.025f,
            0.060f};

        glm::vec3 shallowOceanColor{
            0.025f,
            0.110f,
            0.160f};

        // Microscopic / unresolved surface roughness.
        //
        // Resolved larger waves are handled separately below.
        float oceanRoughness =
            0.10f;

        // =========================================================
        // OCEAN WAVES
        // =========================================================
        //
        // These do NOT displace geometry yet.
        //
        // They perturb the surface normal used for lighting.
        //
        // That is enough to break the perfectly smooth spherical
        // reflection into a much more natural sun-glitter pattern.

        float oceanWaveScale =
            850.0f;

        // Strength of the resolved wave-normal perturbation.
        //
        // Roughly:
        //
        // 0.0  = perfectly smooth sphere
        // 0.1  = gentle
        // 0.2  = clearly visible wave structure
        // 0.4+ = exaggerated / stormy
        //
        // Keep this a bit conservative for now while we validate the
        // new tangent-space wave normal.
        float oceanWaveStrength =
            0.18f;

        // Animation speed multiplier.
        float oceanWaveSpeed =
            0.65f;

        // =========================================================
        // LAND
        // =========================================================

        glm::vec3 lowLandColor{
            0.10f,
            0.22f,
            0.07f};

        glm::vec3 highLandColor{
            0.38f,
            0.31f,
            0.18f};

        float landRoughness =
            0.78f;

        // =========================================================
        // PROCEDURAL SURFACE
        // =========================================================

        float continentScale =
            2.4f;

        float detailScale =
            10.0f;

        // Higher = more ocean.
        float oceanLevel =
            0.56f;

        float coastWidth =
            0.018f;

        // Scales only geometric terrain relief. It is intentionally separate
        // from continent/detail frequencies so a planet can keep the same
        // geography while becoming flatter or more mountainous.
        float terrainReliefScale =
            1.0f;

        // Different procedural planets can share the same shader
        // while producing different geography and wave phases.
        float seed =
            13.37f;
    };
}