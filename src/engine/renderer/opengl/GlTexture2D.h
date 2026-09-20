#pragma once

#include <glad/gl.h>

namespace SpaceSim
{
    class GlTexture2D
    {
    public:
        GlTexture2D(
            int width,
            int height,
            GLenum internalFormat)
            : m_width(
                  width),
              m_height(
                  height),
              m_internalFormat(
                  internalFormat)
        {
            glCreateTextures(
                GL_TEXTURE_2D,
                1,
                &m_texture);

            glTextureStorage2D(
                m_texture,
                1,
                m_internalFormat,
                m_width,
                m_height);

            glTextureParameteri(
                m_texture,
                GL_TEXTURE_MIN_FILTER,
                GL_LINEAR);

            glTextureParameteri(
                m_texture,
                GL_TEXTURE_MAG_FILTER,
                GL_LINEAR);

            glTextureParameteri(
                m_texture,
                GL_TEXTURE_WRAP_S,
                GL_CLAMP_TO_EDGE);

            glTextureParameteri(
                m_texture,
                GL_TEXTURE_WRAP_T,
                GL_CLAMP_TO_EDGE);
        }

        ~GlTexture2D()
        {
            if (m_texture != 0)
            {
                glDeleteTextures(
                    1,
                    &m_texture);
            }
        }

        GlTexture2D(
            const GlTexture2D&) = delete;

        GlTexture2D& operator=(
            const GlTexture2D&) = delete;

        GLuint id() const
        {
            return m_texture;
        }

        int width() const
        {
            return m_width;
        }

        int height() const
        {
            return m_height;
        }

    private:
        GLuint m_texture = 0;

        int m_width = 0;
        int m_height = 0;

        GLenum m_internalFormat =
            GL_RGBA16F;
    };
}