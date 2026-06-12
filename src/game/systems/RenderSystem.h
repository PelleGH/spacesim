#pragma once

#include "renderer/LitMeshRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderDebugView.h"
#include "rendering/MaterialTestSceneRenderer.h"
#include "rendering/MaskedPlanetRenderer.h"
#include "world/GameWorld.h"
#include <memory>
namespace SpaceSim
{
    class RenderSystem
    {
    public:
        void renderSky(GameWorld& world, Renderer& renderer);
        void renderWorld(GameWorld& world, Renderer& renderer);

    private:
        RenderDebugView getDebugView() const;

    private:
        std::unique_ptr<LitMeshRenderer> m_litMeshRenderer;
        std::unique_ptr<MaskedPlanetRenderer> m_maskedPlanetRenderer;
        MaterialTestSceneRenderer m_materialTestSceneRenderer;
    };
}