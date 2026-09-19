#include "renderer/opengl/ShadowMap.h"

#include <stdexcept>

namespace SpaceSim
{
    ShadowMap::ShadowMap(
        int resolution)
        : m_resolution(resolution)
    {
        // Create framebuffer.
        glCreateFramebuffers(
            1,
            &m_framebuffer);

        // Create depth texture.
        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_depthTexture);

        glTextureStorage2D(
            m_depthTexture,
            1,
            GL_DEPTH_COMPONENT32F,
            m_resolution,
            m_resolution);

        glTextureParameteri(
            m_depthTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);

        glTextureParameteri(
            m_depthTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);

        glTextureParameteri(
            m_depthTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_BORDER);

        glTextureParameteri(
            m_depthTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_BORDER);

        const float borderColor[] =
        {
            1.0f,
            1.0f,
            1.0f,
            1.0f
        };

        glTextureParameterfv(
            m_depthTexture,
            GL_TEXTURE_BORDER_COLOR,
            borderColor);

        // Attach the depth texture to the framebuffer.
        glNamedFramebufferTexture(
            m_framebuffer,
            GL_DEPTH_ATTACHMENT,
            m_depthTexture,
            0);

        // This framebuffer has no color attachment.
        glNamedFramebufferDrawBuffer(
            m_framebuffer,
            GL_NONE);

        glNamedFramebufferReadBuffer(
            m_framebuffer,
            GL_NONE);

        const GLenum status =
            glCheckNamedFramebufferStatus(
                m_framebuffer,
                GL_FRAMEBUFFER);

        if (status !=
            GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error(
                "Shadow framebuffer is incomplete.");
        }
    }

    ShadowMap::~ShadowMap()
    {
        if (m_depthTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_depthTexture);
        }

        if (m_framebuffer != 0)
        {
            glDeleteFramebuffers(
                1,
                &m_framebuffer);
        }
    }

    void ShadowMap::bindForWriting() const
    {
        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_framebuffer);

        glViewport(
            0,
            0,
            m_resolution,
            m_resolution);

        glClear(
            GL_DEPTH_BUFFER_BIT);
    }
}