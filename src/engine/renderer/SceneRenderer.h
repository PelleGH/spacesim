#pragma once

#include "renderer/BloomPass.h"
#include "renderer/EnvironmentIbl.h"
#include "renderer/EnvironmentPass.h"
#include "renderer/RenderCamera.h"
#include "renderer/RenderObject.h"

#include "renderer/atmosphere/AtmosphereInstance.h"
#include "renderer/atmosphere/AtmospherePass.h"
#include "renderer/atmosphere/AtmosphereViewLuts.h"

#include "renderer/lighting/DirectionalLight.h"
#include "renderer/lighting/EnvironmentLight.h"

#include "renderer/opengl/GlShader.h"
#include "renderer/opengl/GlTextureCube.h"
#include "renderer/opengl/HdrRenderTarget.h"
#include "renderer/opengl/PostProcessPass.h"
#include "renderer/opengl/ShadowMap.h"

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
            const EnvironmentLight& environment,
            const GlTextureCube& environmentMap,
            const EnvironmentIbl& environmentIbl,
            const std::vector<RenderObject>& objects,
            const AtmosphereInstance* atmosphere = nullptr);


        float exposure() const
        {
            return
                m_exposure;
        }


        void setExposure(
            float exposure)
        {
            m_exposure =
                exposure;
        }


        void setBloomStrength(
            float strength)
        {
            m_bloomStrength =
                strength;
        }


        void setBloomThreshold(
            float threshold)
        {
            m_bloomPass.setThreshold(
                threshold);
        }


        // Controls:
        //
        // - direct sunlight atmospheric attenuation
        // - diffuse atmospheric sky fill

        void setAtmosphereLightingEnabled(
            bool enabled)
        {
            m_atmosphereLightingEnabled =
                enabled;
        }


        bool atmosphereLightingEnabled() const
        {
            return
                m_atmosphereLightingEnabled;
        }


        // Controls atmospheric Sky-View specular reflections.
        //
        // AtmospherePass itself remains enabled.

        void setAtmosphereSpecularEnabled(
            bool enabled)
        {
            m_atmosphereSpecularEnabled =
                enabled;
        }


        bool atmosphereSpecularEnabled() const
        {
            return
                m_atmosphereSpecularEnabled;
        }


    private:
        HdrRenderTarget m_hdrTarget;


        GlShader m_pbrShader;

        GlShader m_shadowShader;


        ShadowMap m_shadowMap;


        EnvironmentPass m_environmentPass;


        // Generated once each frame and shared by both the PBR
        // geometry pass and AtmospherePass.

        AtmosphereViewLuts m_atmosphereViewLuts;


        AtmospherePass m_atmospherePass;


        BloomPass m_bloomPass;

        PostProcessPass m_postProcess;


        float m_exposure =
            1.0f;


        float m_bloomStrength =
            0.12f;


        bool m_atmosphereLightingEnabled =
            true;


        bool m_atmosphereSpecularEnabled =
            true;
    };
}