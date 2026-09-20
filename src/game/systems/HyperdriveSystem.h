#pragma once

#include "game/ecs/GameWorld.h"

#include <string>

namespace SpaceSim
{
    class HyperdriveSystem
    {
    public:
        // Consumes cycle/engage requests once per rendered frame.
        void update(GameWorld& world);

        // Advances alignment and system-scale travel in the fixed simulation.
        void fixedUpdate(GameWorld& world, float dt);

        std::string selectedTargetName(const GameWorld& world) const;
        std::string stateName(const GameWorld& world) const;

    private:
        static void cycleTarget(GameWorld& world, entt::entity ship);
        static bool beginJump(GameWorld& world, entt::entity ship);
        static void updateActiveJump(GameWorld& world, entt::entity ship, float dt);
        static void finishJump(GameWorld& world, entt::entity ship);
    };
}
