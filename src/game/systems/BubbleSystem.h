#pragma once

#include "world/GameWorld.h"

#include <string>

namespace SpaceSim
{
    class BubbleSystem
    {
    public:
        void update(GameWorld& world);

    private:
        bool shouldBecomeLocal(const GlobalObject& object) const;
        bool isInsideBubble(const GameWorld& world, const GlobalObject& object) const;

        entt::entity findLocalEntity(GameWorld& world, const std::string& globalId);
        void spawnLocalEntity(GameWorld& world, const GlobalObject& object);
        void destroyAllLocalObjects(GameWorld& world);
    };
}