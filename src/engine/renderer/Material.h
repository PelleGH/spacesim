#pragma once

#include <raylib.h>

namespace SpaceSim
{
    struct MaterialParams
    {
        Vector3 albedo{ 1.0f, 1.0f, 1.0f };

        // 0 = sharp/glossy, 1 = very rough/matte
        float roughness = 0.7f;

        // 0 = no specular, 1 = strong specular
        float specularStrength = 0.2f;

        // Lets water use darker diffuse while keeping strong specular later.
        float diffuseStrength = 1.0f;
    };
}