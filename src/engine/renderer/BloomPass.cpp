#include "renderer/BloomPass.h"

#include <algorithm>
#include <stdexcept>

namespace SpaceSim
{
    BloomPass::BloomPass()
        : m_extractShader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/renderer/bloom_extract.frag"),
          m_blurShader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/renderer/bloom_blur.frag")
    {
        glCreateVertexArrays(
            1,
            &m_vertexArray);


        m_extractShader.setInt(
            "hdrTexture",
            0);


        m_blurShader.setInt(
            "sourceTexture",
            0);
    }


    BloomPass::~BloomPass()
    {
        destroyTargets();


        if (m_vertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vertexArray);
        }
    }


    void BloomPass::destroyTargets()
    {
        glDeleteTextures(
            2,
            m_textures);


        glDeleteFramebuffers(
            2,
            m_framebuffers);


        m_textures[0] =
            0;

        m_textures[1] =
            0;


        m_framebuffers[0] =
            0;

        m_framebuffers[1] =
            0;


        m_width =
            0;

        m_height =
            0;
    }


    void BloomPass::resize(
        int width,
        int height)
    {
        // Bloom does not need full screen resolution.
        //
        // Half resolution makes it substantially cheaper and
        // naturally gives us a slightly softer result.
        const int bloomWidth =
            std::max(
                1,
                width / 2);


        const int bloomHeight =
            std::max(
                1,
                height / 2);


        if (bloomWidth == m_width &&
            bloomHeight == m_height)
        {
            return;
        }


        destroyTargets();


        m_width =
            bloomWidth;

        m_height =
            bloomHeight;


        glCreateTextures(
            GL_TEXTURE_2D,
            2,
            m_textures);


        glCreateFramebuffers(
            2,
            m_framebuffers);


        for (int i = 0;
             i < 2;
             ++i)
        {
            glTextureStorage2D(
                m_textures[i],
                1,
                GL_RGBA16F,
                m_width,
                m_height);


            glTextureParameteri(
                m_textures[i],
                GL_TEXTURE_MIN_FILTER,
                GL_LINEAR);


            glTextureParameteri(
                m_textures[i],
                GL_TEXTURE_MAG_FILTER,
                GL_LINEAR);


            glTextureParameteri(
                m_textures[i],
                GL_TEXTURE_WRAP_S,
                GL_CLAMP_TO_EDGE);


            glTextureParameteri(
                m_textures[i],
                GL_TEXTURE_WRAP_T,
                GL_CLAMP_TO_EDGE);


            glNamedFramebufferTexture(
                m_framebuffers[i],
                GL_COLOR_ATTACHMENT0,
                m_textures[i],
                0);


            glNamedFramebufferDrawBuffer(
                m_framebuffers[i],
                GL_COLOR_ATTACHMENT0);


            const GLenum status =
                glCheckNamedFramebufferStatus(
                    m_framebuffers[i],
                    GL_FRAMEBUFFER);


            if (status !=
                GL_FRAMEBUFFER_COMPLETE)
            {
                throw std::runtime_error(
                    "Bloom framebuffer is incomplete.");
            }
        }
    }


    GLuint BloomPass::render(
        GLuint hdrSceneTexture,
        int width,
        int height)
    {
        resize(
            width,
            height);


        glDisable(
            GL_DEPTH_TEST);


        glBindVertexArray(
            m_vertexArray);


        // =====================================================
        // PASS 1:
        // Extract only bright HDR pixels.
        // =====================================================

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_framebuffers[0]);


        glViewport(
            0,
            0,
            m_width,
            m_height);


        m_extractShader.use();


        m_extractShader.setFloat(
            "threshold",
            m_threshold);


        glBindTextureUnit(
            0,
            hdrSceneTexture);


        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);


        // =====================================================
        // PASS 2:
        // Repeated horizontal / vertical Gaussian blur.
        // =====================================================

        int sourceIndex =
            0;


        constexpr int blurPassCount =
            10;


        for (int pass = 0;
             pass < blurPassCount;
             ++pass)
        {
            const int targetIndex =
                1 -
                sourceIndex;


            glBindFramebuffer(
                GL_FRAMEBUFFER,
                m_framebuffers[targetIndex]);


            glViewport(
                0,
                0,
                m_width,
                m_height);


            m_blurShader.use();


            // Alternate:
            //
            // horizontal
            // vertical
            // horizontal
            // vertical...
            m_blurShader.setInt(
                "horizontal",
                (pass % 2) == 0
                    ? 1
                    : 0);


            glBindTextureUnit(
                0,
                m_textures[sourceIndex]);


            glDrawArrays(
                GL_TRIANGLES,
                0,
                3);


            sourceIndex =
                targetIndex;
        }


        glBindVertexArray(
            0);


        glEnable(
            GL_DEPTH_TEST);


        return
            m_textures[sourceIndex];
    }
}