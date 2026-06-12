#pragma once

#include "renderer/Lighting.h"
#include "renderer/RenderDebugView.h"
#include "rendering/PlanetSurfaceMapGenerator.h"
#include "world/StarSystem.h"

#include <raylib.h>

#include <string>
#include <unordered_map>

namespace SpaceSim
{
    class MaskedPlanetRenderer
    {
    public:
        MaskedPlanetRenderer();
        ~MaskedPlanetRenderer();

        MaskedPlanetRenderer(const MaskedPlanetRenderer&) = delete;
        MaskedPlanetRenderer& operator=(const MaskedPlanetRenderer&) = delete;

        void drawSphere(
            const GlobalObject& object,
            Vector3 position,
            float radius,
            const LightingEnvironment& lighting,
            const Camera3D& camera,
            RenderDebugView debugView
        );

    private:
        PlanetSurfaceMaps& getSurfaceMaps(const GlobalObject& object);

    private:
        Shader m_shader{};
        Model m_sphereModel{};
        bool m_ready = false;

        std::unordered_map<std::string, PlanetSurfaceMaps> m_surfaceMapCache;
    };
}