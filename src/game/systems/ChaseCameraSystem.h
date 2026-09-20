#pragma once

#include "game/ecs/GameWorld.h"

namespace SpaceSim
{
    class ChaseCameraSystem
    {
    public:
        void update(GameWorld& world, float dt);
        void reset() { m_initialized = false; }

    private:
        bool m_initialized = false;
    };
}
