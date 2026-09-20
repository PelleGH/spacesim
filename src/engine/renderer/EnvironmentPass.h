#pragma once

#include "renderer/RenderCamera.h"
#include "renderer/opengl/GlShader.h"
#include "renderer/opengl/GlTextureCube.h"

#include <glad/gl.h>

namespace SpaceSim
{
    class EnvironmentPass
    {
    public:
        EnvironmentPass();
        ~EnvironmentPass();

        EnvironmentPass(
            const EnvironmentPass&) = delete;

        EnvironmentPass& operator=(
            const EnvironmentPass&) = delete;

        void render(
            const RenderCamera& camera,
            float aspectRatio,
            const GlTextureCube& environment);

    private:
        GlShader m_shader;

        GLuint m_vertexArray = 0;
    };
}