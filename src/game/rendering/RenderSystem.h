#pragma once

#include "game/ecs/GameWorld.h"
#include "renderer/RenderObject.h"
#include "renderer/planet/PlanetRenderObject.h"

#include <vector>

namespace SpaceSim
{
    class GameRenderResources;
    class SceneRenderer;

    class RenderSystem
    {
    public:
        void render(
            GameWorld& world,
            SceneRenderer& renderer,
            const GameRenderResources& resources,
            int width,
            int height);

    private:
        std::vector<PlanetRenderObject> m_planets;
        std::vector<RenderObject> m_objects;
    };
}
