#pragma once

#include "world/StarSystem.h"

#include <entt/entt.hpp>
#include <raylib.h>

namespace SpaceSim
{
    class GameWorld
    {
    public:
        entt::registry registry;

        entt::entity playerShip = entt::null;

        Camera3D camera{};

        StarSystem starSystem;

        DVec3 globalPlayerPosition{};
        DVec3 activeBubbleOrigin{};

        int selectedJumpTarget = -1;
        int activePoi = -1;
    };
}