#include "platform/SdlGlWindow.h"

#include "renderer/RenderCamera.h"
#include "renderer/RenderObject.h"
#include "renderer/SceneRenderer.h"
#include "renderer/lighting/DirectionalLight.h"
#include "renderer/opengl/GpuMesh.h"
#include <glm/ext/matrix_transform.hpp>

#include "renderlab/BoxGenerator.h"

#include <SDL3/SDL.h>

#include <glm/geometric.hpp>

#include <exception>
#include <iostream>
#include <vector>

int main()
{
    try
    {
        // ---------------------------------------------------------
        // Window / platform
        // ---------------------------------------------------------

        SpaceSim::SdlGlWindow window(
            1280,
            720,
            "SpaceSim Renderer Lab");

        // ---------------------------------------------------------
        // Renderer
        // ---------------------------------------------------------

        SpaceSim::SceneRenderer renderer;

        renderer.setExposure(1.0f);

        // ---------------------------------------------------------
        // Temporary ship mesh
        // ---------------------------------------------------------

        const SpaceSim::BoxMeshData shipData =
            SpaceSim::generateBox(
                2.0f,  // width
                0.8f,  // height
                4.0f); // length

        SpaceSim::GpuMesh shipMesh(
            shipData.vertices,
            shipData.indices);

        const SpaceSim::BoxMeshData floorData =
            SpaceSim::generateBox(
                12.0f,
                0.2f,
                12.0f);

        SpaceSim::GpuMesh floorMesh(
            floorData.vertices,
            floorData.indices);

        // ---------------------------------------------------------
        // Camera
        // ---------------------------------------------------------

        SpaceSim::RenderCamera camera;

        camera.position =
            {
                5.0f,
                3.0f,
                7.0f};

        camera.forward =
            glm::normalize(
                -camera.position);

        camera.up =
            {
                0.0f,
                1.0f,
                0.0f};

        camera.verticalFovDegrees =
            45.0f;

        camera.nearPlane =
            0.1f;

        camera.farPlane =
            100.0f;

        // ---------------------------------------------------------
        // Primary star
        // ---------------------------------------------------------

        SpaceSim::DirectionalLight sun;

        sun.direction =
            glm::normalize(
                glm::vec3(
                    -0.5f,
                    0.8f,
                    0.6f));

        sun.radiance =
            {
                15.0f,
                14.5f,
                13.5f};

        // ---------------------------------------------------------
        // Temporary ship render object
        // ---------------------------------------------------------

        SpaceSim::RenderObject ship;

        ship.mesh =
            &shipMesh;

        ship.modelMatrix =
            glm::mat4(1.0f);

        ship.material.baseColor =
            {
                0.25f,
                0.30f,
                0.35f};

        ship.material.metallic =
            0.35f;

        ship.material.roughness =
            0.40f;

        std::vector<SpaceSim::RenderObject> objects;

        objects.push_back(ship);

        // ---------------------------------------------------------
        // Temporary floor for shadow testing
        // ---------------------------------------------------------

        SpaceSim::RenderObject floor;

        floor.mesh =
            &floorMesh;

        floor.modelMatrix =
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(
                    0.0f,
                    -1.2f,
                    0.0f));

        floor.material.baseColor =
            {
                0.15f,
                0.16f,
                0.18f};

        floor.material.metallic =
            0.0f;

        floor.material.roughness =
            0.8f;

        objects.push_back(floor);

        // ---------------------------------------------------------
        // Main loop
        // ---------------------------------------------------------

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

            renderer.render(
                width,
                height,
                camera,
                sun,
                objects);

            window.swapBuffers();
        }

        return 0;
    }
    catch (const std::exception &exception)
    {
        std::cerr
            << "RendererLab fatal error: "
            << exception.what()
            << '\n';

        return 1;
    }
}