#pragma once

#include <glad/gl.h>

#include <algorithm>

namespace SpaceSim
{
    class GlTexture3D
    {
    public:
        GlTexture3D(
            int width,
            int height,
            int depth,
            GLenum internalFormat)
            : m_width(
                  width),
              m_height(
                  height),
              m_depth(
                  depth),
              m_internalFormat(
                  internalFormat),
              m_mipLevels(
                  calculateMipLevelCount(
                      width,
                      height,
                      depth))
        {
            glCreateTextures(
                GL_TEXTURE_3D,
                1,
                &m_texture);


            // Allocate the complete mip chain instead of only level 0.
            glTextureStorage3D(
                m_texture,
                m_mipLevels,
                m_internalFormat,
                m_width,
                m_height,
                m_depth);


            // Trilinear mip filtering:
            //
            // interpolate inside one mip level
            // +
            // interpolate between mip levels
            glTextureParameteri(
                m_texture,
                GL_TEXTURE_MIN_FILTER,
                GL_LINEAR_MIPMAP_LINEAR);


            glTextureParameteri(
                m_texture,
                GL_TEXTURE_MAG_FILTER,
                GL_LINEAR);


            glTextureParameteri(
                m_texture,
                GL_TEXTURE_WRAP_S,
                GL_REPEAT);


            glTextureParameteri(
                m_texture,
                GL_TEXTURE_WRAP_T,
                GL_REPEAT);


            glTextureParameteri(
                m_texture,
                GL_TEXTURE_WRAP_R,
                GL_REPEAT);
        }


        ~GlTexture3D()
        {
            if (m_texture != 0)
            {
                glDeleteTextures(
                    1,
                    &m_texture);


                m_texture =
                    0;
            }
        }


        GlTexture3D(
            const GlTexture3D&) = delete;


        GlTexture3D& operator=(
            const GlTexture3D&) = delete;


        GLuint id() const
        {
            return
                m_texture;
        }


        int width() const
        {
            return
                m_width;
        }


        int height() const
        {
            return
                m_height;
        }


        int depth() const
        {
            return
                m_depth;
        }


        int mipLevels() const
        {
            return
                m_mipLevels;
        }


    private:
        static int calculateMipLevelCount(
            int width,
            int height,
            int depth)
        {
            int largestDimension =
                std::max(
                    width,
                    std::max(
                        height,
                        depth));


            int levels =
                1;


            while (largestDimension > 1)
            {
                largestDimension /=
                    2;


                ++levels;
            }


            return
                levels;
        }


        GLuint m_texture =
            0;


        int m_width =
            0;


        int m_height =
            0;


        int m_depth =
            0;


        int m_mipLevels =
            1;


        GLenum m_internalFormat =
            GL_RGBA8;
    };
}