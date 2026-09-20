#include "renderer/opengl/PostProcessPass.h"

namespace SpaceSim
{
    PostProcessPass::PostProcessPass()
        : m_shader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/renderer/tonemap.frag")
    {
        glCreateVertexArrays(
            1,
            &m_vertexArray);


        m_shader.setInt(
            "hdrTexture",
            0);


        m_shader.setInt(
            "bloomTexture",
            1);
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
        GLuint bloomTexture,
        float exposure,
        float bloomStrength)
    {
        glDisable(
            GL_DEPTH_TEST);


        m_shader.use();


        m_shader.setFloat(
            "exposure",
            exposure);


        m_shader.setFloat(
            "bloomStrength",
            bloomStrength);


        glBindTextureUnit(
            0,
            hdrTexture);


        glBindTextureUnit(
            1,
            bloomTexture);


        glBindVertexArray(
            m_vertexArray);


        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);


        glBindVertexArray(
            0);


        glEnable(
            GL_DEPTH_TEST);
    }
}