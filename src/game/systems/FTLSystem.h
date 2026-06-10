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

        void beginFTLTravel(GameWorld& world);
        void updateFTLTravel(GameWorld& world, float dt);
        void arriveFromFTLTravel(GameWorld& world);
        void cancelFTLTravel(GameWorld& world);
    };
}