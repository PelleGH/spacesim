#pragma once

#include "renderer/opengl/GlShader.h"

#include <glad/gl.h>


namespace SpaceSim
{
    class PostProcessPass
    {
    public:
        PostProcessPass();

        ~PostProcessPass();


        PostProcessPass(
            const PostProcessPass&) = delete;


        PostProcessPass& operator=(
            const PostProcessPass&) = delete;


        void render(
            GLuint hdrTexture,
            GLuint bloomTexture,
            GLuint autoExposureTexture,
            bool autoExposureEnabled,
            float exposureCompensation,
            float bloomStrength);


    private:
        GlShader m_shader;


        GLuint m_vertexArray =
            0;
    };
}