#pragma once

#include "world/GameWorld.h"

namespace SpaceSim
{
    class CameraSystem
    {
    public:
        void initialize(GameWorld& world);
        void update(GameWorld& world, float dt);
    };
}