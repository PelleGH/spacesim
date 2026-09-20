#include "renderer/opengl/HdrRenderTarget.h"

#include <stdexcept>

namespace SpaceSim
{
    HdrRenderTarget::~HdrRenderTarget()
    {
        destroy();
    }


    void HdrRenderTarget::destroy()
    {
        if (m_depthTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_depthTexture);

            m_depthTexture = 0;
        }


        if (m_linearDepthTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_linearDepthTexture);

            m_linearDepthTexture = 0;
        }


        if (m_colorTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_colorTexture);

            m_colorTexture = 0;
        }


        if (m_framebuffer != 0)
        {
            glDeleteFramebuffers(
                1,
                &m_framebuffer);

            m_framebuffer = 0;
        }


        m_width = 0;
        m_height = 0;
    }


    void HdrRenderTarget::resize(
        int width,
        int height)
    {
        if (width <= 0 ||
            height <= 0)
        {
            return;
        }


        if (width == m_width &&
            height == m_height)
        {
            return;
        }


        destroy();


        m_width = width;
        m_height = height;


        glCreateFramebuffers(
            1,
            &m_framebuffer);


        // =========================================================
        // HDR COLOR
        // =========================================================

        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_colorTexture);


        glTextureStorage2D(
            m_colorTexture,
            1,
            GL_RGBA16F,
            width,
            height);


        glTextureParameteri(
            m_colorTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_colorTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_colorTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_colorTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glNamedFramebufferTexture(
            m_framebuffer,
            GL_COLOR_ATTACHMENT0,
            m_colorTexture,
            0);


        // =========================================================
        // LINEAR CAMERA DISTANCE
        // =========================================================
        //
        // Perspective hardware depth becomes extremely compressed
        // near 1.0 when the near plane is tiny.
        //
        // Instead, geometry writes its actual distance from the
        // camera into this R32F texture.
        //
        // Examples:
        //
        // nearby ship:
        //     2.5
        //
        // planet:
        //     94.3
        //
        // no geometry:
        //     0.0

        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_linearDepthTexture);


        glTextureStorage2D(
            m_linearDepthTexture,
            1,
            GL_R32F,
            width,
            height);


        glTextureParameteri(
            m_linearDepthTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);


        glTextureParameteri(
            m_linearDepthTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);


        glTextureParameteri(
            m_linearDepthTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_linearDepthTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glNamedFramebufferTexture(
            m_framebuffer,
            GL_COLOR_ATTACHMENT1,
            m_linearDepthTexture,
            0);


        // =========================================================
        // HARDWARE DEPTH
        // =========================================================
        //
        // We still need this for ordinary triangle rasterization.
        //
        // The atmosphere simply no longer tries to recover physical
        // distance from it.

        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_depthTexture);


        glTextureStorage2D(
            m_depthTexture,
            1,
            GL_DEPTH_COMPONENT32F,
            width,
            height);


        glTextureParameteri(
            m_depthTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);


        glTextureParameteri(
            m_depthTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);


        glNamedFramebufferTexture(
            m_framebuffer,
            GL_DEPTH_ATTACHMENT,
            m_depthTexture,
            0);


        // PBR fragments now write:
        //
        // location 0 -> HDR color
        // location 1 -> linear distance

        const GLenum drawBuffers[]
        {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1
        };


        glNamedFramebufferDrawBuffers(
            m_framebuffer,
            2,
            drawBuffers);


        const GLenum status =
            glCheckNamedFramebufferStatus(
                m_framebuffer,
                GL_FRAMEBUFFER);


        if (status !=
            GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error(
                "HDR framebuffer is incomplete.");
        }
    }


    void HdrRenderTarget::bind() const
    {
        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_framebuffer);
    }
}