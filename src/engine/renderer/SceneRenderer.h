#pragma once

#include "renderer/AutoExposurePass.h"
#include "renderer/BloomPass.h"
#include "renderer/EnvironmentIbl.h"
#include "renderer/EnvironmentPass.h"
#include "renderer/RenderCamera.h"
#include "renderer/RenderObject.h"
#include "renderer/StarPass.h"

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

#include "renderer/planet/PlanetPass.h"
#include "renderer/planet/PlanetRenderObject.h"

#include <vector>


namespace SpaceSim
{
    class SceneRenderer
    {
    public:
        SceneRenderer();


        // Existing-compatible overload.
        //
        // Anything that doesn't use the dedicated planet path yet
        // can continue calling SceneRenderer exactly as before.
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


        // New dedicated-planet overload.
        void render(
            int width,
            int height,
            const RenderCamera& camera,
            const DirectionalLight& sun,
            const EnvironmentLight& environment,
            const GlTextureCube& environmentMap,
            const EnvironmentIbl& environmentIbl,
            const std::vector<PlanetRenderObject>& planets,
            const std::vector<RenderObject>& objects,
            const AtmosphereInstance* atmosphere = nullptr);


        float exposure() const
        {
            return
                m_exposureCompensation;
        }


        void setExposure(
            float exposureCompensation)
        {
            m_exposureCompensation =
                exposureCompensation;
        }


        void setAutoExposureEnabled(
            bool enabled)
        {
            m_autoExposureEnabled =
                enabled;
        }


        bool autoExposureEnabled() const
        {
            return
                m_autoExposureEnabled;
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


        StarPass m_starPass;


        PlanetPass m_planetPass;


        AtmosphereViewLuts m_atmosphereViewLuts;


        AtmospherePass m_atmospherePass;


        AutoExposurePass m_autoExposurePass;


        BloomPass m_bloomPass;


        PostProcessPass m_postProcess;


        float m_exposureCompensation =
            1.0f;


        float m_bloomStrength =
            0.12f;


        bool m_autoExposureEnabled =
            true;


        bool m_atmosphereLightingEnabled =
            true;


        bool m_atmosphereSpecularEnabled =
            true;
    };
}