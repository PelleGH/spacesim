#pragma once

#include "world/GameWorld.h"

namespace SpaceSim
{
    class DistantBodyRenderer
    {
    public:
        void render(const GameWorld& world);
    };
}