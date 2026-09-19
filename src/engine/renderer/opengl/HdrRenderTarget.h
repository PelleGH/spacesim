#pragma once

#include <glad/gl.h>

namespace SpaceSim
{
    class HdrRenderTarget
    {
    public:
        HdrRenderTarget() = default;
        ~HdrRenderTarget();

        HdrRenderTarget(
            const HdrRenderTarget&) = delete;

        HdrRenderTarget& operator=(
            const HdrRenderTarget&) = delete;

        void resize(
            int width,
            int height);

        void bind() const;

        GLuint colorTexture() const
        {
            return m_colorTexture;
        }

    private:
        void destroy();

        GLuint m_framebuffer = 0;
        GLuint m_colorTexture = 0;
        GLuint m_depthTexture = 0;

        int m_width = 0;
        int m_height = 0;
    };
}