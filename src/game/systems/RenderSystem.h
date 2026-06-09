#pragma once

#include "renderer/Renderer.h"
#include "world/GameWorld.h"

namespace SpaceSim
{
    class RenderSystem
    {
    public:
        void renderSky(GameWorld& world, Renderer& renderer);
        void renderWorld(GameWorld& world, Renderer& renderer);
    };
}