#pragma once

#include "renderer/LitMeshRenderer.h"

namespace SpaceSim
{
    class MaterialTestSceneRenderer
    {
    public:
        void render(
            LitMeshRenderer& litMeshRenderer,
            const Camera3D& camera,
            RenderDebugView debugView
        );
    };
}