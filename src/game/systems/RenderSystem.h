#pragma once

#include "renderer/Renderer.h"
#include "rendering/DistantBodyRenderer.h"
#include "rendering/SpaceBackgroundRenderer.h"
#include "rendering/PrototypeMeshRenderer.h"
#include "rendering/PlanetSurfacePatch.h"
#include "rendering/AdaptivePlanetRenderer.h"
#include "world/GameWorld.h"

namespace SpaceSim
{
    class RenderSystem
    {
    public:
        void renderSky(GameWorld& world, Renderer& renderer);
        void renderWorld(GameWorld& world, Renderer& renderer);

        void renderAtmosphereOverlay(GameWorld& world, Renderer& renderer);

    private:
        OrbitalPlanetRenderInfo getActiveOrbitalPlanetRenderInfo(
            const GameWorld& world) const;

        void drawActiveOrbitalPlanet(GameWorld& world, bool drawSurface = true);

        SpaceBackgroundRenderer m_spaceBackgroundRenderer;
        DistantBodyRenderer m_distantBodyRenderer;
        PrototypeMeshRenderer m_prototypeMeshRenderer;
        PlanetSurfacePatch m_surfacePatch;
        AdaptivePlanetRenderer m_adaptivePlanet;
        bool m_useAdaptivePlanet = true;
        bool m_showPlanetLayers = true;
        // 0 = unlit albedo, 1 = geometric normals, 2 = shared scene lighting.
        int m_terrainLightingMode = 2;
    };
}
