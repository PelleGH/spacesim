#include "renderer/opengl/GlTextureCube.h"

namespace SpaceSim
{
    GlTextureCube::GlTextureCube(
        int resolution,
        int mipLevels)
        : m_resolution(
              resolution),
          m_mipLevels(
              mipLevels)
    {
        glCreateTextures(
            GL_TEXTURE_CUBE_MAP,
            1,
            &m_texture);

        glTextureStorage2D(
            m_texture,
            m_mipLevels,
            GL_RGBA16F,
            m_resolution,
            m_resolution);

        glTextureParameteri(
            m_texture,
            GL_TEXTURE_MIN_FILTER,
            m_mipLevels > 1
                ? GL_LINEAR_MIPMAP_LINEAR
                : GL_LINEAR);

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

        glTextureParameteri(
            m_texture,
            GL_TEXTURE_WRAP_R,
            GL_CLAMP_TO_EDGE);

        glTextureParameteri(
            m_texture,
            GL_TEXTURE_BASE_LEVEL,
            0);

        glTextureParameteri(
            m_texture,
            GL_TEXTURE_MAX_LEVEL,
            m_mipLevels - 1);
    }

    GlTextureCube::~GlTextureCube()
    {
        if (m_texture != 0)
        {
            glDeleteTextures(
                1,
                &m_texture);
        }
    }
}