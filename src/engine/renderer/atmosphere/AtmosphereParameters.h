#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct AtmosphereParameters
    {
        // =========================================================
        // PLANET GEOMETRY
        // =========================================================

        // Radius where the atmosphere begins.
        //
        // For Earth this is approximately the planet surface.
        float bottomRadiusKm =
            6360.0f;

        // Artificial top of the atmosphere.
        //
        // Real atmospheres fade continuously forever, but at some
        // point the density becomes negligible enough that we can
        // stop evaluating it.
        float topRadiusKm =
            6460.0f;


        // =========================================================
        // SEMANTIC ALTITUDE BANDS
        // =========================================================
        //
        // These are NOT extra rendered spheres.
        //
        // They are common physical altitude references that later systems
        // can share:
        //
        // clouds
        // weather
        // atmospheric flight
        // drag
        // re-entry
        // visual effects
        //
        // The actual Hillaire scattering atmosphere remains continuous.

        float troposphereTopAltitudeKm =
            12.0f;

        float stratosphereTopAltitudeKm =
            50.0f;

        float mesosphereTopAltitudeKm =
            85.0f;


        // =========================================================
        // RAYLEIGH SCATTERING
        // =========================================================

        // Small air molecules.
        //
        // Blue has the largest coefficient, which is why shorter
        // wavelengths are scattered much more strongly.
        //
        // Units: 1 / km
        glm::vec3 rayleighScatteringPerKm
        {
            0.005802f,
            0.013558f,
            0.033100f
        };

        // Atmospheric density falls approximately exponentially.
        //
        // Roughly:
        //
        // density = exp(-height / scaleHeight)
        float rayleighScaleHeightKm =
            8.0f;


        // =========================================================
        // MIE SCATTERING
        // =========================================================

        // Larger aerosol particles such as dust/haze.
        //
        // Unlike Rayleigh scattering, this is approximately neutral
        // across visible RGB wavelengths.
        glm::vec3 mieScatteringPerKm
        {
            0.003996f,
            0.003996f,
            0.003996f
        };

        // Total Mie extinction.
        //
        // Extinction includes light removed from the ray by both
        // scattering and absorption.
        glm::vec3 mieExtinctionPerKm
        {
            0.004440f,
            0.004440f,
            0.004440f
        };

        float mieScaleHeightKm =
            1.2f;

        // Used later when we calculate visible Mie scattering.
        //
        // Positive values strongly favor forward scattering,
        // producing the bright haze around the sun.
        float mieAnisotropy =
            0.8f;


        // =========================================================
        // OZONE ABSORPTION
        // =========================================================

        // Ozone doesn't primarily scatter visible light here.
        // It absorbs different wavelengths at different rates.
        glm::vec3 ozoneAbsorptionPerKm
        {
            0.000650f,
            0.001881f,
            0.000085f
        };

        // Simplified ozone layer profile.
        float ozoneCenterHeightKm =
            25.0f;

        float ozoneHalfWidthKm =
            15.0f;


        // =========================================================
        // GROUND
        // =========================================================

        // Used later for multiple scattering / ground bounce.
        glm::vec3 groundAlbedo
        {
            0.30f,
            0.30f,
            0.30f
        };
    };


    inline AtmosphereParameters makeEarthLikeAtmosphere()
    {
        return AtmosphereParameters{};
    }
}