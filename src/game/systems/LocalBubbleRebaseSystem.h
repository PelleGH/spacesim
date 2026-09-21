#pragma once

#include "game/ecs/GameWorld.h"

namespace SpaceSim
{
    // Keeps local gameplay coordinates numerically close to the origin while
    // preserving every local entity's global position. Global/system-scale
    // travel remains represented by GameWorld::localBubbleOriginMeters.
    class LocalBubbleRebaseSystem
    {
    public:
        void fixedUpdate(GameWorld& world);

    private:
        static constexpr double RebaseDistanceMeters = 10000.0;
    };
}
