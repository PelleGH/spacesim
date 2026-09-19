#include "renderer/opengl/PostProcessPass.h"

namespace SpaceSim
{
    PostProcessPass::PostProcessPass()
        : m_shader(
            "data/shaders/renderer/fullscreen.vert",
            "data/shaders/renderer/tonemap.frag")
    {
        // Core OpenGL requires a VAO to be bound,
        // even though our fullscreen triangle has
        // no actual vertex buffer.
        glCreateVertexArrays(
            1,
            &m_vertexArray);

        const GLint textureLocation =
            glGetUniformLocation(
                m_shader.id(),
                "hdrTexture");

        glProgramUniform1i(
            m_shader.id(),
            textureLocation,
            0);
    }

    PostProcessPass::~PostProcessPass()
    {
        if (m_vertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vertexArray);
        }
    }

    void PostProcessPass::render(
        GLuint hdrTexture,
        float exposure)
    {
        glDisable(GL_DEPTH_TEST);

        m_shader.use();

        const GLint exposureLocation =
            glGetUniformLocation(
                m_shader.id(),
                "exposure");

        glProgramUniform1f(
            m_shader.id(),
            exposureLocation,
            exposure);

        glBindTextureUnit(
            0,
            hdrTexture);

        glBindVertexArray(
            m_vertexArray);

        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);

        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
    }
}