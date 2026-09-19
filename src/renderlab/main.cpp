#include "platform/SdlGlWindow.h"

#include "renderer/opengl/GlShader.h"
#include "renderer/opengl/GpuMesh.h"
#include "renderer/opengl/HdrRenderTarget.h"
#include "renderer/opengl/PostProcessPass.h"

#include "renderlab/SphereGenerator.h"

#include <glad/gl.h>

#include <SDL3/SDL.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <exception>
#include <iostream>

namespace
{
    struct TestMaterial
    {
        glm::vec3 position;
        glm::vec3 baseColor;

        float metallic;
        float roughness;
    };
}

int main()
{
    try
    {
        SpaceSim::SdlGlWindow window(
            1280,
            720,
            "SpaceSim PBR Lighting Lab");

        SpaceSim::HdrRenderTarget hdrTarget;

        SpaceSim::PostProcessPass postProcess;

        SpaceSim::GlShader pbrShader(
            "data/shaders/renderer/pbr.vert",
            "data/shaders/renderer/pbr.frag");

        const SpaceSim::SphereMeshData sphereData =
            SpaceSim::generateSphere(
                64,
                32);

        SpaceSim::GpuMesh sphere(
            sphereData.vertices,
            sphereData.indices);

        const glm::vec3 cameraPosition(
            0.0f,
            0.0f,
            8.0f);

        const glm::vec3 sunDirection =
            glm::normalize(
                glm::vec3(
                    -0.5f,
                    0.7f,
                    0.8f));

        // HDR light intensity.
        const glm::vec3 sunRadiance(
            15.0f,
            14.5f,
            13.5f);

        const TestMaterial materials[] =
        {
            // Rough dielectric
            {
                glm::vec3(-1.7f, 1.4f, 0.0f),
                glm::vec3(0.55f, 0.08f, 0.04f),
                0.0f,
                0.80f
            },

            // Smooth dielectric
            {
                glm::vec3(1.7f, 1.4f, 0.0f),
                glm::vec3(0.55f, 0.08f, 0.04f),
                0.0f,
                0.15f
            },

            // Rough metal
            {
                glm::vec3(-1.7f, -1.4f, 0.0f),
                glm::vec3(0.90f, 0.55f, 0.25f),
                1.0f,
                0.65f
            },

            // Smooth metal
            {
                glm::vec3(1.7f, -1.4f, 0.0f),
                glm::vec3(0.90f, 0.55f, 0.25f),
                1.0f,
                0.12f
            }
        };

        while (window.processEvents())
        {
            const int width =
                window.pixelWidth();

            const int height =
                window.pixelHeight();

            if (width <= 0 ||
                height <= 0)
            {
                SDL_Delay(10);
                continue;
            }

            hdrTarget.resize(
                width,
                height);

            const float aspectRatio =
                static_cast<float>(width) /
                static_cast<float>(height);

            const glm::mat4 view =
                glm::lookAt(
                    cameraPosition,
                    glm::vec3(0.0f),
                    glm::vec3(
                        0.0f,
                        1.0f,
                        0.0f));

            const glm::mat4 projection =
                glm::perspective(
                    glm::radians(45.0f),
                    aspectRatio,
                    0.1f,
                    100.0f);

            // =====================================================
            // HDR 3D SCENE
            // =====================================================

            hdrTarget.bind();

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

            pbrShader.use();

            pbrShader.setMat4(
                "view",
                view);

            pbrShader.setMat4(
                "projection",
                projection);

            pbrShader.setVec3(
                "cameraPosition",
                cameraPosition);

            pbrShader.setVec3(
                "sunDirection",
                sunDirection);

            pbrShader.setVec3(
                "sunRadiance",
                sunRadiance);

            for (const TestMaterial& material :
                 materials)
            {
                glm::mat4 model(1.0f);

                model =
                    glm::translate(
                        model,
                        material.position);

                pbrShader.setMat4(
                    "model",
                    model);

                pbrShader.setVec3(
                    "baseColor",
                    material.baseColor);

                pbrShader.setFloat(
                    "metallic",
                    material.metallic);

                pbrShader.setFloat(
                    "roughness",
                    material.roughness);

                sphere.draw();
            }

            // =====================================================
            // HDR -> DISPLAY
            // =====================================================

            glBindFramebuffer(
                GL_FRAMEBUFFER,
                0);

            glViewport(
                0,
                0,
                width,
                height);

            postProcess.render(
                hdrTarget.colorTexture(),
                1.0f);

            window.swapBuffers();
        }

        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr
            << "RendererLab fatal error: "
            << exception.what()
            << '\n';

        return 1;
    }
}