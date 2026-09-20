#pragma once

#include "game/ecs/GameWorld.h"

namespace SpaceSim
{
    // Temporary bootstrap for the first modern game executable. The entities it
    // creates are normal ECS entities; the hard-coded scene can later be replaced
    // by data loading without changing the application/render loop.
    void createPrototypeWorld(GameWorld& world);
}
