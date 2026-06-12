#pragma once

#include <raylib.h>

namespace SpaceSim
{
    struct DirectionalLight
    {
        // from shaded object to light source, should be normalized
        Vector3 directionToLight{ -0.5f, 0.35f, -0.8f };

        Vector3 color{ 1.0f, 0.96f, 0.88f };
        float intensity = 1.0f;
    };

    struct LightingEnvironment
    {
        DirectionalLight sun;

        Vector3 ambientColor{ 1.0f, 1.0f, 1.0f };
        float ambientIntensity = 0.025f;
    };
}