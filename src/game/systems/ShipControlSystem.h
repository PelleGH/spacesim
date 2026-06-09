#pragma once

#include "world/GameWorld.h"

#include <raylib.h>

namespace SpaceSim
{
    class ShipControlSystem
    {
    public:
        void update(GameWorld& world, float dt);

        Vector2 getVirtualStick() const { return m_virtualStick; }
        float getControlRadius() const { return m_controlRadius; }

    private:
        Vector2 m_virtualStick{ 0.0f, 0.0f };
        float m_controlRadius = 70.0f;
    };
}