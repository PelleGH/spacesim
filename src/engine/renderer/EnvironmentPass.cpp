#include "renderer/EnvironmentPass.h"

#include <glm/mat4x4.hpp>

namespace SpaceSim
{
    EnvironmentPass::EnvironmentPass()
        : m_shader(
            "data/shaders/renderer/environment.vert",
            "data/shaders/renderer/environment.frag")
    {
        glCreateVertexArrays(
            1,
            &m_vertexArray);

        m_shader.setInt(
            "environmentMap",
            2);
    }

    EnvironmentPass::~EnvironmentPass()
    {
        if (m_vertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vertexArray);
        }
    }

    void EnvironmentPass::render(
        const RenderCamera& camera,
        float aspectRatio,
        const GlTextureCube& environment)
    {
        const glm::mat4 view =
            camera.viewMatrix();

        const glm::mat4 projection =
            camera.projectionMatrix(
                aspectRatio);

        const glm::mat4 inverseViewProjection =
            glm::inverse(
                projection *
                view);

        m_shader.use();

        m_shader.setMat4(
            "inverseViewProjection",
            inverseViewProjection);

        m_shader.setVec3(
            "cameraPosition",
            camera.position);

        glBindTextureUnit(
            2,
            environment.id());

        glDisable(GL_DEPTH_TEST);

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