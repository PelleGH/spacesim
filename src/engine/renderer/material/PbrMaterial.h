#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct PbrMaterial
    {
        glm::vec3 baseColor
        {
            0.5f,
            0.5f,
            0.5f
        };

        float metallic =
            0.0f;

        float roughness =
            0.5f;


        // Color produced directly by the material itself.
        glm::vec3 emissiveColor
        {
            0.0f,
            0.0f,
            0.0f
        };

        // HDR multiplier.
        //
        // 0   = not emissive
        // 1   = approximately ordinary brightness
        // 20+ = very bright engine/light/etc.
        float emissiveStrength =
            0.0f;
    };
}