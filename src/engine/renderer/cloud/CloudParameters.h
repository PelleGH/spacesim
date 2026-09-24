#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct CloudParameters
    {
        // =========================================================
        // VERTICAL CLOUD REGION
        // =========================================================

        float baseAltitudeKm =
            1.5f;

        float topAltitudeKm =
            12.0f;

        float boundaryFadeKm =
            0.75f;

        // =========================================================
        // GLOBAL WEATHER
        // =========================================================

        // 0.5 is neutral.
        //
        // Lower:
        //     generally clearer planet
        //
        // Higher:
        //     generally cloudier planet
        float coverage =
            0.52f;

        // =========================================================
        // VOLUMETRIC SHAPE
        // =========================================================

        // Large cloud masses visible from orbit.
        float coarseShapePeriodKm =
            1200.0f;

        // Smaller local cloud structure.
        float baseShapePeriodKm =
            220.0f;

        float densityMultiplier =
            1.0f;

        // =========================================================
        // LOCAL SHAPE LOD
        // =========================================================

        // Fine cloud structure fades out as one screen pixel begins
        // representing larger physical distances.
        float localShapeFadeStartFootprintKm =
            1.5f;

        float localShapeFadeEndFootprintKm =
            14.0f;

        // =========================================================
        // OPTICAL PROPERTIES
        // =========================================================

        // Extinction controls how quickly light is removed while
        // travelling through cloud.
        //
        // Used by BOTH:
        //
        // camera -> cloud
        // cloud  -> sun
        float extinctionPerKm =
            0.14f;

        // Water clouds scatter almost all visible light instead of
        // absorbing it.
        //
        // Keep this very close to white.
        glm::vec3 scatteringAlbedo{
            0.999f,
            0.999f,
            0.999f};

        // =========================================================
        // CLOUD SELF-SHADOWING
        // =========================================================

        // Multiplier applied to extinction during the light march.
        //
        // Less than 1 prevents this first approximation from making
        // every dense cloud interior pitch black.
        float shadowExtinctionMultiplier =
            0.70f;

        // =========================================================
        // PHASE FUNCTION
        // =========================================================
        //
        // Cloud droplets strongly favor forward scattering.
        //
        // g = 0:
        //     isotropic
        //
        // g > 0:
        //     forward scattering
        //
        // g < 0:
        //     backward scattering

        float forwardScatteringG =
            0.70f;

        float backwardScatteringG =
            -0.20f;

        // Most of the response comes from the forward lobe.
        float forwardScatteringWeight =
            0.85f;

        // Useful while tuning HDR brightness without changing the
        // physical sun radiance shared by the rest of the renderer.
        float lightingIntensity =
            1.0f;

        bool valid() const
        {
            return baseAltitudeKm >=
                       0.0f &&
                   topAltitudeKm >
                       baseAltitudeKm &&
                   boundaryFadeKm >=
                       0.0f &&
                   coverage >=
                       0.0f &&
                   coverage <=
                       1.0f &&
                   coarseShapePeriodKm >
                       baseShapePeriodKm &&
                   baseShapePeriodKm >
                       0.0f &&
                   densityMultiplier >=
                       0.0f &&
                   localShapeFadeStartFootprintKm >=
                       0.0f &&
                   localShapeFadeEndFootprintKm >
                       localShapeFadeStartFootprintKm &&
                   extinctionPerKm >=
                       0.0f &&
                   shadowExtinctionMultiplier >=
                       0.0f &&
                   forwardScatteringG >
                       -0.99f &&
                   forwardScatteringG <
                       0.99f &&
                   backwardScatteringG >
                       -0.99f &&
                   backwardScatteringG <
                       0.99f &&
                   forwardScatteringWeight >=
                       0.0f &&
                   forwardScatteringWeight <=
                       1.0f &&
                   lightingIntensity >=
                       0.0f;
        }
    };

    inline CloudParameters makeEarthLikeClouds()
    {
        return CloudParameters{};
    }
}