#pragma once

#include "world/GameWorld.h"

#include <raylib.h>

namespace SpaceSim
{
    class CameraSystem
    {
    public:
        void initialize(GameWorld& world);
        void update(GameWorld& world, float dt);

    private:
        Vector2 m_freeLook{ 0.0f, 0.0f };

        float m_freeLookSensitivity = 0.003f;
        float m_freeLookReturnSpeed = 6.0f;
        float m_maxFreeLookYaw = 1.2f;
        float m_maxFreeLookPitch = 0.75f;
    };
}