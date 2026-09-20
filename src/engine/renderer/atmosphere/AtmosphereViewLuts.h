#pragma once

#include "renderer/RenderCamera.h"
#include "renderer/atmosphere/AtmosphereInstance.h"
#include "renderer/lighting/DirectionalLight.h"
#include "renderer/opengl/GlComputeShader.h"

#include <glad/gl.h>

namespace SpaceSim
{
    class AtmosphereViewLuts
    {
    public:
        AtmosphereViewLuts();
        ~AtmosphereViewLuts();

        AtmosphereViewLuts(
            const AtmosphereViewLuts&) = delete;

        AtmosphereViewLuts& operator=(
            const AtmosphereViewLuts&) = delete;

        void update(
            const RenderCamera& camera,
            float aspectRatio,
            const DirectionalLight& sun,
            const AtmosphereInstance& atmosphere);

        GLuint skyViewTexture() const
        {
            return
                m_skyViewTexture;
        }

        GLuint skyReflectionTexture() const
        {
            return
                m_skyReflectionTexture;
        }

        GLuint aerialScatteringTexture() const
        {
            return
                m_aerialScatteringTexture;
        }

        GLuint aerialTransmittanceTexture() const
        {
            return
                m_aerialTransmittanceTexture;
        }

    private:
        void createTextures();

        void destroyTextures();

        void setCommonUniforms(
            const GlComputeShader& shader,
            const RenderCamera& camera,
            float aspectRatio,
            const DirectionalLight& sun,
            const AtmosphereInstance& atmosphere) const;

        GlComputeShader m_skyViewShader;

        GlComputeShader m_skyReflectionPrefilterShader;

        GlComputeShader m_aerialPerspectiveShader;

        GLuint m_skyViewTexture =
            0;

        GLuint m_skyReflectionTexture =
            0;

        GLuint m_aerialScatteringTexture =
            0;

        GLuint m_aerialTransmittanceTexture =
            0;

        static constexpr int SkyWidth =
            320;

        static constexpr int SkyHeight =
            180;

        static constexpr int SkyMipLevels =
            9;

        static constexpr int AerialWidth =
            320;

        static constexpr int AerialHeight =
            180;

        static constexpr int AerialDepth =
            32;
    };
}