#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct DirectionalLight
    {
        // Direction FROM the surface TOWARD the light.
        glm::vec3 direction
        {
            0.0f,
            1.0f,
            0.0f
        };

        // Linear HDR radiance.
        glm::vec3 radiance
        {
            1.0f
        };
    };
}