#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct CloudParameters
    {
        // ---------------------------------------------------------
        // Physical cloud volume
        // ---------------------------------------------------------

        float baseAltitudeKm = 1.5f;

        // The cloud shell now has enough room for deep convection.
        //
        // Individual cloud families still decide how much of this
        // volume they actually occupy:
        //
        // stratus  -> low few km
        // cumulus  -> roughly <= 10-12 km
        // storms   -> can approach 16 km
        float topAltitudeKm = 16.0f;

        float boundaryFadeKm = 0.75f;


        // ---------------------------------------------------------
        // Global weather
        // ---------------------------------------------------------

        float coverage = 0.52f;


        // ---------------------------------------------------------
        // Default / large-scale planetary shape
        // ---------------------------------------------------------

        float coarseShapePeriodKm = 160.0f;
        float baseShapePeriodKm = 40.0f;

        float densityMultiplier = 1.0f;


        // ---------------------------------------------------------
        // Distance LOD
        // ---------------------------------------------------------

        float localShapeFadeStartFootprintKm = 1.5f;
        float localShapeFadeEndFootprintKm = 14.0f;


        // ---------------------------------------------------------
        // Optical properties
        // ---------------------------------------------------------

        float extinctionPerKm = 0.14f;

        glm::vec3 scatteringAlbedo{
            0.999f,
            0.999f,
            0.999f
        };

        float shadowExtinctionMultiplier = 0.70f;


        // ---------------------------------------------------------
        // Phase function
        // ---------------------------------------------------------

        float forwardScatteringG = 0.70f;
        float backwardScatteringG = -0.20f;

        float forwardScatteringWeight = 0.85f;


        // ---------------------------------------------------------
        // Final light multiplier
        // ---------------------------------------------------------

        float lightingIntensity = 1.0f;


        bool valid() const
        {
            return
                baseAltitudeKm >= 0.0f &&
                topAltitudeKm > baseAltitudeKm &&
                boundaryFadeKm >= 0.0f &&

                coverage >= 0.0f &&
                coverage <= 1.0f &&

                coarseShapePeriodKm > baseShapePeriodKm &&
                baseShapePeriodKm > 0.0f &&

                densityMultiplier >= 0.0f &&

                localShapeFadeStartFootprintKm >= 0.0f &&
                localShapeFadeEndFootprintKm >
                    localShapeFadeStartFootprintKm &&

                extinctionPerKm >= 0.0f &&
                shadowExtinctionMultiplier >= 0.0f &&

                forwardScatteringG > -0.99f &&
                forwardScatteringG < 0.99f &&

                backwardScatteringG > -0.99f &&
                backwardScatteringG < 0.99f &&

                forwardScatteringWeight >= 0.0f &&
                forwardScatteringWeight <= 1.0f &&

                lightingIntensity >= 0.0f;
        }
    };


    inline CloudParameters makeEarthLikeClouds()
    {
        return CloudParameters{};
    }
}