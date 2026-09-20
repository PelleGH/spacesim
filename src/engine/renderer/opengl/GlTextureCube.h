#pragma once

#include <glad/gl.h>

namespace SpaceSim
{
    class GlTextureCube
    {
    public:
        explicit GlTextureCube(
            int resolution,
            int mipLevels = 1);

        ~GlTextureCube();

        GlTextureCube(
            const GlTextureCube&) = delete;

        GlTextureCube& operator=(
            const GlTextureCube&) = delete;

        GLuint id() const
        {
            return m_texture;
        }

        int resolution() const
        {
            return m_resolution;
        }

        int mipLevels() const
        {
            return m_mipLevels;
        }

    private:
        GLuint m_texture = 0;

        int m_resolution = 0;
        int m_mipLevels = 1;
    };
}