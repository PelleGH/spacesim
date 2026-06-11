#pragma once

#include <raylib.h>

namespace SpaceSim
{
    enum class RenderableType
    {
        Ship,
        Satellite
    };

    struct RenderableComponent
    {
        RenderableType type = RenderableType::Ship;
        Color color = WHITE;
    };
}