#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct EnvironmentLight
    {
        glm::vec3 diffuseMultiplier
        {
            1.0f,
            1.0f,
            1.0f
        };

        glm::vec3 specularMultiplier
        {
            1.0f,
            1.0f,
            1.0f
        };
    };
}