#pragma once

#include "renderer/opengl/GlShader.h"
#include "renderer/opengl/GlTexture2D.h"

#include <glad/gl.h>

namespace SpaceSim
{
    class AtmosphereDebugPass
    {
    public:
        AtmosphereDebugPass()
            : m_shader(
                  "data/shaders/renderer/fullscreen.vert",
                  "data/shaders/atmosphere/texture_debug.frag")
        {
            glCreateVertexArrays(
                1,
                &m_vertexArray);


            m_shader.setInt(
                "debugTexture",
                0);
        }


        ~AtmosphereDebugPass()
        {
            if (m_vertexArray != 0)
            {
                glDeleteVertexArrays(
                    1,
                    &m_vertexArray);
            }
        }


        AtmosphereDebugPass(
            const AtmosphereDebugPass&) = delete;


        AtmosphereDebugPass& operator=(
            const AtmosphereDebugPass&) = delete;


        void setExposure(
            float exposure)
        {
            m_exposure =
                exposure;
        }


        void render(
            const GlTexture2D& texture,
            int width,
            int height)
        {
            glBindFramebuffer(
                GL_FRAMEBUFFER,
                0);


            glViewport(
                0,
                0,
                width,
                height);


            glDisable(
                GL_DEPTH_TEST);


            m_shader.use();


            m_shader.setFloat(
                "exposure",
                m_exposure);


            glBindTextureUnit(
                0,
                texture.id());


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


    private:
        GlShader m_shader;


        GLuint m_vertexArray =
            0;


        float m_exposure =
            0.25f;
    };
}