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


    private:
        void createTextures();

        void destroyTextures();


        void setCommonUniforms(
            const GlComputeShader& shader,
            const RenderCamera& camera,
            float aspectRatio,
            const DirectionalLight& sun,
            const AtmosphereInstance& atmosphere) const;


        // =========================================================
        // VIEW-DEPENDENT ATMOSPHERE
        // =========================================================
        //
        // We intentionally no longer keep a 3D aerial-perspective
        // volume here.
        //
        // Aerial perspective is now integrated continuously in
        // atmosphere_planet.frag using the actual camera-to-fragment
        // path distance.
        //
        // These two textures remain:
        //
        // 1. Sky View
        //      directional atmosphere seen by the camera
        //
        // 2. Sky Reflection
        //      GGX-prefiltered version used for reflective surfaces

        GlComputeShader m_skyViewShader;

        GlComputeShader m_skyReflectionPrefilterShader;


        GLuint m_skyViewTexture =
            0;

        GLuint m_skyReflectionTexture =
            0;


        static constexpr int SkyWidth =
            320;

        static constexpr int SkyHeight =
            180;

        static constexpr int SkyMipLevels =
            9;
    };
}