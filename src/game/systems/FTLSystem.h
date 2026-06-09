#pragma once

#include "world/GameWorld.h"

namespace SpaceSim
{
    class FTLSystem
    {
    public:
        void update(GameWorld& world, float dt);

    private:
        void selectNextJumpTarget(GameWorld& world);
        void jumpToSelectedTarget(GameWorld& world);
    };
}