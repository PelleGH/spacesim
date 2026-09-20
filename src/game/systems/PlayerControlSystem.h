#pragma once

#include "game/ecs/GameWorld.h"

namespace SpaceSim
{
    class SdlInput;

    class PlayerControlSystem
    {
    public:
        void update(GameWorld& world, const SdlInput& input, float frameDt);
    };
}
