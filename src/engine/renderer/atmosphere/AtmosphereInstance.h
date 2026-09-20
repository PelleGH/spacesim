#pragma once

#include "renderer/atmosphere/AtmosphereLuts.h"
#include "renderer/atmosphere/AtmosphereParameters.h"

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct AtmosphereInstance
    {
        const AtmosphereParameters* parameters =
            nullptr;

        const AtmosphereLuts* luts =
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
    };
}