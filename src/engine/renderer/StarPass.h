#pragma once

#include "renderer/RenderCamera.h"
#include "renderer/lighting/DirectionalLight.h"
#include "renderer/opengl/GlShader.h"

#include <glad/gl.h>


namespace SpaceSim
{
    class StarPass
    {
    public:
        StarPass();

        ~StarPass();


        StarPass(
            const StarPass&) = delete;


        StarPass& operator=(
            const StarPass&) = delete;


        void render(
            const RenderCamera& camera,
            float aspectRatio,
            const DirectionalLight& star);


    private:
        // Fullscreen shader for the primary stellar disk.
        GlShader m_shader;


        // Point-rendering shader for the distant background stars.
        GlShader m_starfieldShader;


        // Fullscreen triangle VAO used by the primary star disk.
        GLuint m_vertexArray =
            0;


        // Distant-star catalog.
        GLuint m_starfieldVertexArray =
            0;


        GLuint m_starfieldVertexBuffer =
            0;


        GLsizei m_starCount =
            0;
    };
}