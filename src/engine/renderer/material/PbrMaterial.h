#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct PbrMaterial
    {
        glm::vec3 baseColor
        {
            0.5f
        };

        float metallic = 0.0f;
        float roughness = 0.5f;

        glm::vec3 emissive
        {
            0.0f
        };

        float emissiveStrength = 0.0f;
    };
}