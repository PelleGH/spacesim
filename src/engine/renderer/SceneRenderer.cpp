#include "renderer/SceneRenderer.h"

#include "renderer/opengl/GpuMesh.h"

#include <glad/gl.h>

namespace SpaceSim
{
    SceneRenderer::SceneRenderer()
        : m_pbrShader(
            "data/shaders/renderer/pbr.vert",
            "data/shaders/renderer/pbr.frag")
    {
    }

    void SceneRenderer::render(
        int width,
        int height,
        const RenderCamera& camera,
        const DirectionalLight& sun,
        const std::vector<RenderObject>& objects)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        m_hdrTarget.resize(
            width,
            height);

        const float aspect =
            static_cast<float>(width) /
            static_cast<float>(height);

        // =========================================================
        // HDR SCENE PASS
        // =========================================================

        m_hdrTarget.bind();

        glViewport(
            0,
            0,
            width,
            height);

        const GLfloat background[4]
        {
            0.001f,
            0.0015f,
            0.003f,
            1.0f
        };

        glClearBufferfv(
            GL_COLOR,
            0,
            background);

        glClear(
            GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        m_pbrShader.use();

        m_pbrShader.setMat4(
            "view",
            camera.viewMatrix());

        m_pbrShader.setMat4(
            "projection",
            camera.projectionMatrix(aspect));

        m_pbrShader.setVec3(
            "cameraPosition",
            camera.position);

        m_pbrShader.setVec3(
            "sunDirection",
            sun.direction);

        m_pbrShader.setVec3(
            "sunRadiance",
            sun.radiance);

        for (const RenderObject& object : objects)
        {
            if (!object.mesh)
            {
                continue;
            }

            m_pbrShader.setMat4(
                "model",
                object.modelMatrix);

            m_pbrShader.setVec3(
                "baseColor",
                object.material.baseColor);

            m_pbrShader.setFloat(
                "metallic",
                object.material.metallic);

            m_pbrShader.setFloat(
                "roughness",
                object.material.roughness);

            object.mesh->draw();
        }

        // =========================================================
        // DISPLAY PASS
        // =========================================================

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            0);

        glViewport(
            0,
            0,
            width,
            height);

        m_postProcess.render(
            m_hdrTarget.colorTexture(),
            m_exposure);
    }
}