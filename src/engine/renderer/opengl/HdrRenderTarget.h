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

        // Actual camera-to-fragment distance in world units.
        //
        // This is separate from the normal hardware depth buffer
        // because perspective depth is highly non-linear.
        GLuint linearDepthTexture() const
        {
            return m_linearDepthTexture;
        }

        // Normal hardware depth used for rasterization/depth testing.
        GLuint depthTexture() const
        {
            return m_depthTexture;
        }

    private:
        void destroy();

        GLuint m_framebuffer = 0;

        GLuint m_colorTexture = 0;

        GLuint m_linearDepthTexture = 0;

        GLuint m_depthTexture = 0;

        int m_width = 0;
        int m_height = 0;
    };
}