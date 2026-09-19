#pragma once

#include "renderer/RenderCamera.h"
#include "renderer/RenderObject.h"
#include "renderer/lighting/DirectionalLight.h"

#include "renderer/opengl/GlShader.h"
#include "renderer/opengl/HdrRenderTarget.h"
#include "renderer/opengl/PostProcessPass.h"

#include <vector>

namespace SpaceSim
{
    class SceneRenderer
    {
    public:
        SceneRenderer();

        void render(
            int width,
            int height,
            const RenderCamera& camera,
            const DirectionalLight& sun,
            const std::vector<RenderObject>& objects);

        float exposure() const
        {
            return m_exposure;
        }

        void setExposure(float exposure)
        {
            m_exposure = exposure;
        }

    private:
        HdrRenderTarget m_hdrTarget;

        GlShader m_pbrShader;

        PostProcessPass m_postProcess;

        float m_exposure = 1.0f;
    };
}