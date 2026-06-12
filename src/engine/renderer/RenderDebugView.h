#pragma once

namespace SpaceSim
{
    enum class RenderDebugView
    {
        Final = 0,
        Albedo = 1,
        Normal = 2,
        NdotL = 3,
        Diffuse = 4,
        Specular = 5,
        Roughness = 6,
        MaterialMask = 7
    };
}