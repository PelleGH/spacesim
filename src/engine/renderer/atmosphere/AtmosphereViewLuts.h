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

        GlComputeShader m_aerialPerspectiveShader;


        GLuint m_skyViewTexture =
            0;


        GLuint m_aerialScatteringTexture =
            0;


        GLuint m_aerialTransmittanceTexture =
            0;


        // =========================================================
        // SKY VIEW
        // =========================================================
        //
        // This mainly represents atmospheric background radiance.
        //
        // 320 x 180 is already reasonably smooth when linearly
        // filtered up to a 1280 x 720 output.

        static constexpr int SkyWidth =
            320;


        static constexpr int SkyHeight =
            180;


        // =========================================================
        // AERIAL PERSPECTIVE
        // =========================================================
        //
        // Previous test:
        //
        //     64 x 36
        //
        // At 1280 x 720 that meant one XY cell represented:
        //
        //     20 x 20 screen pixels
        //
        // which was visibly exposed around the planet.
        //
        // 160 x 90 reduces that to:
        //
        //     8 x 8 screen pixels
        //
        // before trilinear filtering.
        //
        // This is still deliberately lower-resolution than the
        // framebuffer because the atmosphere changes relatively
        // smoothly across the image.

        static constexpr int AerialWidth =
            160;


        static constexpr int AerialHeight =
            90;


        // Z represents distance through the atmosphere.
        //
        // Keep this at 32 for now because the screenshot is showing
        // obvious XY quantization, not depth-slice quantization.
        static constexpr int AerialDepth =
            32;
    };
}