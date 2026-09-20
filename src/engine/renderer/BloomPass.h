#pragma once

#include "renderer/opengl/GlShader.h"

#include <glad/gl.h>

namespace SpaceSim
{
    class BloomPass
    {
    public:
        BloomPass();

        ~BloomPass();

        BloomPass(
            const BloomPass&) = delete;

        BloomPass& operator=(
            const BloomPass&) = delete;

        GLuint render(
            GLuint hdrSceneTexture,
            int width,
            int height);

        void setThreshold(
            float threshold)
        {
            m_threshold =
                threshold;
        }

    private:
        void resize(
            int width,
            int height);

        void destroyTargets();


        GlShader m_extractShader;

        GlShader m_blurShader;


        GLuint m_vertexArray =
            0;


        GLuint m_framebuffers[2]
        {
            0,
            0
        };


        GLuint m_textures[2]
        {
            0,
            0
        };


        int m_width =
            0;

        int m_height =
            0;


        float m_threshold =
            1.5f;
    };
}