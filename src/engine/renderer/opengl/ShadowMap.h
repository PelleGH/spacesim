#pragma once

#include <glad/gl.h>

namespace SpaceSim
{
    class ShadowMap
    {
    public:
        explicit ShadowMap(
            int resolution = 2048);

        ~ShadowMap();

        ShadowMap(
            const ShadowMap&) = delete;

        ShadowMap& operator=(
            const ShadowMap&) = delete;

        void bindForWriting() const;

        GLuint depthTexture() const
        {
            return m_depthTexture;
        }

        int resolution() const
        {
            return m_resolution;
        }

    private:
        GLuint m_framebuffer = 0;
        GLuint m_depthTexture = 0;

        int m_resolution = 2048;
    };
}