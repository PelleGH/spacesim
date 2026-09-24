#pragma once

#include "renderer/atmosphere/AtmosphereLuts.h"
#include "renderer/atmosphere/AtmosphereParameters.h"
#include "renderer/cloud/CloudParameters.h"

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct AtmosphereInstance
    {
        const AtmosphereParameters* parameters =
            nullptr;

        const AtmosphereLuts* luts =
            nullptr;


        // Optional cloud configuration for the same host planet.
        //
        // ECS still owns clouds separately from the atmosphere.
        //
        // This pointer simply bundles the active planet's volumetric render
        // information for this particular frame.
        const CloudParameters* cloudParameters =
            nullptr;


        // Planet center in renderer/world coordinates.
        glm::vec3 planetCenterWorld
        {
            0.0f,
            0.0f,
            0.0f
        };


        // Renderer-space radius corresponding to
        // AtmosphereParameters::bottomRadiusKm.
        float planetRadiusWorld =
            1.0f;


        bool valid() const
        {
            return
                parameters != nullptr
                &&
                luts != nullptr
                &&
                planetRadiusWorld > 0.0f;
        }


        bool cloudsValid() const
        {
            return
                valid()
                &&
                cloudParameters != nullptr
                &&
                cloudParameters->valid();
        }
    };
}