#pragma once

#include "game/ecs/GameWorld.h"

namespace SpaceSim
{
    class SdlInput;

    // Temporary debug camera system. It proves the ECS/system loop while ship
    // movement is rebuilt; later the gameplay camera can replace it cleanly.
    class FreeCameraSystem
    {
    public:
        void update(GameWorld& world, const SdlInput& input, float dt);

    private:
        void applyPreset(GameWorld& world, int preset);
    };
}
