#pragma once

#include "renderer/Renderer.h"
#include "rendering/DistantBodyRenderer.h"
#include "rendering/SpaceBackgroundRenderer.h"
#include "rendering/PrototypeMeshRenderer.h"
#include "world/GameWorld.h"

namespace SpaceSim
{
    class RenderSystem
    {
    public:
        void renderSky(GameWorld& world, Renderer& renderer);
        void renderWorld(GameWorld& world, Renderer& renderer);

    private:
        SpaceBackgroundRenderer m_spaceBackgroundRenderer;
        DistantBodyRenderer m_distantBodyRenderer;
        PrototypeMeshRenderer m_prototypeMeshRenderer;
    };
}