#pragma once

#include <raylib.h>

namespace SpaceSim
{
    class SpaceBackgroundRenderer
    {
    public:
        void render(const Camera3D& camera);
    };
}