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


        m_shader.setInt(
            "autoExposureTexture",
            2);
    }


    PostProcessPass::~PostProcessPass()
    {
        if (m_vertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vertexArray);


            m_vertexArray =
                0;
        }
    }


    void PostProcessPass::render(
        GLuint hdrTexture,
        GLuint bloomTexture,
        GLuint autoExposureTexture,
        bool autoExposureEnabled,
        float exposureCompensation,
        float bloomStrength)
    {
        glDisable(
            GL_DEPTH_TEST);


        m_shader.use();


        m_shader.setInt(
            "autoExposureEnabled",
            autoExposureEnabled
                ?
                1
                :
                0);


        m_shader.setFloat(
            "exposureCompensation",
            exposureCompensation);


        m_shader.setFloat(
            "bloomStrength",
            bloomStrength);


        glBindTextureUnit(
            0,
            hdrTexture);


        glBindTextureUnit(
            1,
            bloomTexture);


        glBindTextureUnit(
            2,
            autoExposureTexture);


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