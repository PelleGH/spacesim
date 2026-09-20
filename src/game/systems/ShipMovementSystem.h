#pragma once

#include "game/ecs/GameWorld.h"

namespace SpaceSim
{
    class ShipMovementSystem
    {
    public:
        void fixedUpdate(GameWorld& world, float dt);
    };
}
