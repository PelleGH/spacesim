#pragma once

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


namespace SpaceSim
{
    // =========================================================
    // ONE STABLE CLOUD FORMATION
    // =========================================================
    //
    // This is cloud DATA.
    //
    // It deliberately contains no:
    //
    //     camera distance
    //     ray step size
    //     render resolution
    //     LOD mode
    //
    // Those belong to the renderer.
    //
    // The underlying formation therefore does not change simply
    // because the camera moves.

    struct CloudFormation
    {
        bool enabled =
            true;


        // Unit vector from planet center toward this formation.
        //
        // Cloud2TestPass locks this once when we first approach
        // Earth, then leaves it fixed.
        glm::vec3 planetDirection
        {
            0.0f,
            1.0f,
            0.0f
        };


        // Physical kilometres above sea level.
        float baseAltitudeKm =
            1.5f;


        // Horizontal radius of the controlled test formation.
        float horizontalRadiusKm =
            8.5f;


        // Vertical development.
        //
        // 1.5 km base + 9 km height gives a cloud top around
        // 10.5 km.
        float heightKm =
            9.0f;


        // Stable procedural seed.
        float seed =
            13.37f;


        // Density multiplier belongs to the physical formation,
        // not to camera LOD.
        float densityMultiplier =
            1.0f;


        // Approximate extinction coefficient through cloud water.
        //
        // We will tune this later once the shape is proven.
        float extinctionPerKm =
            0.34f;


        bool valid() const
        {
            return
                enabled &&
                glm::length(
                    planetDirection) >
                    0.001f &&
                baseAltitudeKm >=
                    0.0f &&
                horizontalRadiusKm >
                    0.0f &&
                heightKm >
                    0.0f &&
                densityMultiplier >=
                    0.0f &&
                extinctionPerKm >=
                    0.0f;
        }
    };


    inline CloudFormation makeTestCumulusFormation()
    {
        return
            CloudFormation{};
    }
}