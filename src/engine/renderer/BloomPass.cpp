#include "renderer/BloomPass.h"

#include <algorithm>
#include <stdexcept>


namespace SpaceSim
{
    BloomPass::BloomPass()
        : m_extractShader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/renderer/bloom_extract.frag"),

          m_downsampleShader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/renderer/bloom_downsample.frag"),

          m_upsampleShader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/renderer/bloom_upsample.frag")
    {
        glCreateVertexArrays(
            1,
            &m_vertexArray);


        m_extractShader.setInt(
            "hdrTexture",
            0);


        m_downsampleShader.setInt(
            "sourceTexture",
            0);


        m_upsampleShader.setInt(
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


            m_vertexArray =
                0;
        }
    }


    void BloomPass::destroyTargets()
    {
        glDeleteTextures(
            BloomLevelCount,
            m_textures.data());


        glDeleteFramebuffers(
            BloomLevelCount,
            m_framebuffers.data());


        m_textures.fill(
            0);


        m_framebuffers.fill(
            0);


        m_levelWidths.fill(
            0);


        m_levelHeights.fill(
            0);


        m_sourceWidth =
            0;


        m_sourceHeight =
            0;
    }


    void BloomPass::resize(
        int width,
        int height)
    {
        if (width <= 0 ||
            height <= 0)
        {
            return;
        }


        if (width == m_sourceWidth &&
            height == m_sourceHeight)
        {
            return;
        }


        destroyTargets();


        m_sourceWidth =
            width;


        m_sourceHeight =
            height;


        glCreateTextures(
            GL_TEXTURE_2D,
            BloomLevelCount,
            m_textures.data());


        glCreateFramebuffers(
            BloomLevelCount,
            m_framebuffers.data());


        // =========================================================
        // BUILD BLOOM PYRAMID
        // =========================================================
        //
        // Level 0 starts at half screen resolution.
        //
        // Every later level halves each dimension again.
        //
        // At 1280x720 this becomes approximately:
        //
        //     640x360
        //     320x180
        //     160x90
        //      80x45
        //      40x22
        //      20x11

        int levelWidth =
            std::max(
                1,
                width / 2);


        int levelHeight =
            std::max(
                1,
                height / 2);


        for (int level = 0;
             level < BloomLevelCount;
             ++level)
        {
            m_levelWidths[level] =
                levelWidth;


            m_levelHeights[level] =
                levelHeight;


            glTextureStorage2D(
                m_textures[level],
                1,
                GL_RGBA16F,
                levelWidth,
                levelHeight);


            glTextureParameteri(
                m_textures[level],
                GL_TEXTURE_MIN_FILTER,
                GL_LINEAR);


            glTextureParameteri(
                m_textures[level],
                GL_TEXTURE_MAG_FILTER,
                GL_LINEAR);


            glTextureParameteri(
                m_textures[level],
                GL_TEXTURE_WRAP_S,
                GL_CLAMP_TO_EDGE);


            glTextureParameteri(
                m_textures[level],
                GL_TEXTURE_WRAP_T,
                GL_CLAMP_TO_EDGE);


            glNamedFramebufferTexture(
                m_framebuffers[level],
                GL_COLOR_ATTACHMENT0,
                m_textures[level],
                0);


            glNamedFramebufferDrawBuffer(
                m_framebuffers[level],
                GL_COLOR_ATTACHMENT0);


            const GLenum status =
                glCheckNamedFramebufferStatus(
                    m_framebuffers[level],
                    GL_FRAMEBUFFER);


            if (status !=
                GL_FRAMEBUFFER_COMPLETE)
            {
                throw std::runtime_error(
                    "Bloom framebuffer is incomplete.");
            }


            levelWidth =
                std::max(
                    1,
                    levelWidth / 2);


            levelHeight =
                std::max(
                    1,
                    levelHeight / 2);
        }
    }


    GLuint BloomPass::render(
        GLuint hdrSceneTexture,
        int width,
        int height)
    {
        if (hdrSceneTexture == 0 ||
            width <= 0 ||
            height <= 0)
        {
            return
                0;
        }


        resize(
            width,
            height);


        glDisable(
            GL_DEPTH_TEST);


        glDepthMask(
            GL_FALSE);


        glDisable(
            GL_BLEND);


        glBindVertexArray(
            m_vertexArray);


        // =========================================================
        // PASS 1:
        // BRIGHT EXTRACTION
        // =========================================================
        //
        // Extract HDR energy which should contribute to bloom.
        //
        // We immediately write at half resolution because bloom
        // doesn't need full-resolution detail.

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_framebuffers[0]);


        glViewport(
            0,
            0,
            m_levelWidths[0],
            m_levelHeights[0]);


        m_extractShader.use();


        m_extractShader.setFloat(
            "threshold",
            m_threshold);


        m_extractShader.setFloat(
            "softKnee",
            m_softKnee);


        glBindTextureUnit(
            0,
            hdrSceneTexture);


        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);


        // =========================================================
        // PASS 2:
        // DOWNSAMPLE PYRAMID
        // =========================================================
        //
        // Each level contains a progressively broader version of
        // the bright scene.
        //
        // The smallest levels are what eventually create the very
        // wide, subtle halo around an intense source.

        m_downsampleShader.use();


        for (int level = 1;
             level < BloomLevelCount;
             ++level)
        {
            glBindFramebuffer(
                GL_FRAMEBUFFER,
                m_framebuffers[level]);


            glViewport(
                0,
                0,
                m_levelWidths[level],
                m_levelHeights[level]);


            glBindTextureUnit(
                0,
                m_textures[
                    level - 1]);


            glDrawArrays(
                GL_TRIANGLES,
                0,
                3);
        }


        // =========================================================
        // PASS 3:
        // UPSAMPLE + ADD
        // =========================================================
        //
        // Start at the broadest level and add it into the next
        // larger level.
        //
        // Continue upward until level 0 contains all bloom scales.

        glEnable(
            GL_BLEND);


        glBlendEquation(
            GL_FUNC_ADD);


        glBlendFunc(
            GL_ONE,
            GL_ONE);


        m_upsampleShader.use();


        m_upsampleShader.setFloat(
            "filterRadius",
            1.0f);


        for (int level =
                 BloomLevelCount - 1;
             level > 0;
             --level)
        {
            const int targetLevel =
                level - 1;


            glBindFramebuffer(
                GL_FRAMEBUFFER,
                m_framebuffers[
                    targetLevel]);


            glViewport(
                0,
                0,
                m_levelWidths[
                    targetLevel],
                m_levelHeights[
                    targetLevel]);


            glBindTextureUnit(
                0,
                m_textures[
                    level]);


            glDrawArrays(
                GL_TRIANGLES,
                0,
                3);
        }


        // =========================================================
        // RESTORE STATE
        // =========================================================

        glDisable(
            GL_BLEND);


        glBindVertexArray(
            0);


        glDepthMask(
            GL_TRUE);


        glEnable(
            GL_DEPTH_TEST);


        // Level 0 now contains:
        //
        //     tight glow
        //     +
        //     medium glow
        //     +
        //     broad halo
        //
        // PostProcessPass adds this back to the original HDR scene.

        return
            m_textures[0];
    }
}