#include "renderer/SceneRenderer.h"

#include "renderer/opengl/GpuMesh.h"

#include <glad/gl.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <cmath>

namespace SpaceSim
{
    SceneRenderer::SceneRenderer()
        : m_pbrShader(
              "data/shaders/renderer/pbr.vert",
              "data/shaders/renderer/pbr.frag"),
          m_shadowShader(
              "data/shaders/renderer/shadow.vert",
              "data/shaders/renderer/shadow.frag"),
          m_shadowMap(2048)
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

        // Make sure our HDR framebuffer matches the window size.
        m_hdrTarget.resize(
            width,
            height);

        const float aspect =
            static_cast<float>(width) /
            static_cast<float>(height);

        // =========================================================
        // BUILD THE SUN CAMERA
        // =========================================================

        const glm::vec3 sceneCenter(
            0.0f,
            0.0f,
            0.0f);

        // sun.direction means:
        //
        //     surface -> sun
        //
        // Therefore the virtual camera representing the sun
        // should sit IN the sun direction and look back at
        // the scene.
        const glm::vec3 lightPosition =
            sceneCenter +
            sun.direction * 20.0f;

        glm::vec3 lightUp(
            0.0f,
            1.0f,
            0.0f);

        // Avoid glm::lookAt degenerating if the sun happens
        // to point almost exactly along the Y axis.
        if (std::abs(
                glm::dot(
                    sun.direction,
                    lightUp)) > 0.95f)
        {
            lightUp =
            {
                0.0f,
                0.0f,
                1.0f
            };
        }

        const glm::mat4 lightView =
            glm::lookAt(
                lightPosition,
                sceneCenter,
                lightUp);

        // Directional lights use an orthographic projection.
        const glm::mat4 lightProjection =
            glm::ortho(
                -10.0f,
                 10.0f,
                -10.0f,
                 10.0f,
                 0.1f,
                 50.0f);

        const glm::mat4 lightSpaceMatrix =
            lightProjection *
            lightView;

        // =========================================================
        // PASS 1: SHADOW MAP
        // =========================================================

        m_shadowMap.bindForWriting();

        glEnable(GL_DEPTH_TEST);

        m_shadowShader.use();

        // THIS WAS MISSING IN YOUR CURRENT FILE.
        m_shadowShader.setMat4(
            "lightSpaceMatrix",
            lightSpaceMatrix);

        for (const RenderObject& object : objects)
        {
            if (!object.mesh)
            {
                continue;
            }

            m_shadowShader.setMat4(
                "model",
                object.modelMatrix);

            object.mesh->draw();
        }

        // =========================================================
        // PASS 2: HDR PBR SCENE
        // =========================================================

        // THIS IS THE IMPORTANT FIX.
        //
        // The shadow framebuffer is still bound after the
        // previous pass, so we MUST switch back to the HDR
        // framebuffer before drawing the normal scene.
        m_hdrTarget.bind();

        glViewport(
            0,
            0,
            width,
            height);

        const GLfloat background[4] =
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

        // Camera.
        m_pbrShader.setMat4(
            "view",
            camera.viewMatrix());

        m_pbrShader.setMat4(
            "projection",
            camera.projectionMatrix(aspect));

        m_pbrShader.setVec3(
            "cameraPosition",
            camera.position);

        // Sun.
        m_pbrShader.setVec3(
            "sunDirection",
            sun.direction);

        m_pbrShader.setVec3(
            "sunRadiance",
            sun.radiance);

        // Shadow information.
        m_pbrShader.setMat4(
            "lightSpaceMatrix",
            lightSpaceMatrix);

        m_pbrShader.setInt(
            "shadowMap",
            1);

        glBindTextureUnit(
            1,
            m_shadowMap.depthTexture());

        // Draw scene objects.
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
        // PASS 3: HDR -> SCREEN
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