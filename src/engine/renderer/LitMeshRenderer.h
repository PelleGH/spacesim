#pragma once

#include "renderer/Lighting.h"
#include "renderer/Material.h"
#include "renderer/RenderDebugView.h"

#include <raylib.h>

namespace SpaceSim
{
    class LitMeshRenderer
    {
    public:
        LitMeshRenderer();
        ~LitMeshRenderer();

        LitMeshRenderer(const LitMeshRenderer&) = delete;
        LitMeshRenderer& operator=(const LitMeshRenderer&) = delete;

        void drawSphere(
            Vector3 position,
            float radius,
            const MaterialParams& material,
            const LightingEnvironment& lighting,
            const Camera3D& camera,
            RenderDebugView debugView
        );

    private:
        Shader m_shader{};
        Model m_sphereModel{};
        bool m_ready = false;
    };
}