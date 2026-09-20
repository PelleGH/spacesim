#pragma once

#include "renderer/opengl/GlShader.h"

#include <glad/gl.h>

#include <array>


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


        void setSoftKnee(
            float softKnee)
        {
            m_softKnee =
                softKnee;
        }


    private:
        static constexpr int BloomLevelCount =
            6;


        void resize(
            int width,
            int height);


        void destroyTargets();


        GlShader m_extractShader;

        GlShader m_downsampleShader;

        GlShader m_upsampleShader;


        GLuint m_vertexArray =
            0;


        std::array<GLuint, BloomLevelCount> m_framebuffers
        {
        };


        std::array<GLuint, BloomLevelCount> m_textures
        {
        };


        std::array<int, BloomLevelCount> m_levelWidths
        {
        };


        std::array<int, BloomLevelCount> m_levelHeights
        {
        };


        int m_sourceWidth =
            0;


        int m_sourceHeight =
            0;


        // Scene-referred HDR brightness at which bloom begins.
        float m_threshold =
            8.0f;


        // Fraction of the threshold used for the soft transition
        // into bloom.
        //
        // 0 = hard threshold
        // 1 = very gradual transition
        float m_softKnee =
            0.5f;
    };
}