#include "platform/SdlGlWindow.h"

#include "renderer/EnvironmentIbl.h"
#include "renderer/RenderCamera.h"
#include "renderer/RenderObject.h"
#include "renderer/SceneRenderer.h"

#include "renderer/atmosphere/AtmosphereInstance.h"
#include "renderer/atmosphere/AtmosphereLuts.h"
#include "renderer/atmosphere/AtmosphereParameters.h"

#include "renderer/lighting/DirectionalLight.h"
#include "renderer/lighting/EnvironmentLight.h"

#include "renderer/opengl/GlTextureCube.h"
#include "renderer/opengl/GpuMesh.h"

#include "renderlab/BoxGenerator.h"
#include "renderlab/SphereGenerator.h"
#include "renderlab/TestEnvironment.h"

#include <SDL3/SDL.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>
#include <exception>
#include <iostream>
#include <vector>


int main()
{
    try
    {
        // =========================================================
        // WINDOW
        // =========================================================

        SpaceSim::SdlGlWindow window(
            1280,
            720,
            "SpaceSim Atmosphere Lab");


        // =========================================================
        // ATMOSPHERE
        // =========================================================

        SpaceSim::AtmosphereParameters atmosphere =
            SpaceSim::makeEarthLikeAtmosphere();


        atmosphere.groundAlbedo =
        {
            0.10f,
            0.14f,
            0.18f
        };


        SpaceSim::AtmosphereLuts atmosphereLuts(
            atmosphere);


        // =========================================================
        // WORLD SCALE
        // =========================================================

        constexpr float planetRadiusWorld =
            50.0f;


        const float kmPerWorldUnit =
            atmosphere.bottomRadiusKm
            /
            planetRadiusWorld;


        const float worldUnitsPerKm =
            1.0f
            /
            kmPerWorldUnit;


        const glm::vec3 planetCenterWorld
        {
            0.0f,
            0.0f,
            0.0f
        };


        SpaceSim::AtmosphereInstance atmosphereInstance;


        atmosphereInstance.parameters =
            &atmosphere;


        atmosphereInstance.luts =
            &atmosphereLuts;


        atmosphereInstance.planetCenterWorld =
            planetCenterWorld;


        atmosphereInstance.planetRadiusWorld =
            planetRadiusWorld;


        // =========================================================
        // RENDERER
        // =========================================================

        SpaceSim::SceneRenderer renderer;


        renderer.setExposure(
            0.7f);


        renderer.setBloomStrength(
            0.0f);


        bool atmosphericLightingEnabled =
            true;


        bool atmosphericSpecularEnabled =
            true;


        renderer.setAtmosphereLightingEnabled(
            atmosphericLightingEnabled);


        renderer.setAtmosphereSpecularEnabled(
            atmosphericSpecularEnabled);


        // =========================================================
        // ENVIRONMENT
        // =========================================================

        SpaceSim::GlTextureCube environmentMap(
            64);


        SpaceSim::fillTestEnvironment(
            environmentMap);


        SpaceSim::EnvironmentIbl environmentIbl(
            environmentMap);


        SpaceSim::EnvironmentLight environment;


        // Disable the old fake/test IBL.
        //
        // We want atmospheric lighting to be easy to isolate.

        environment.diffuseMultiplier =
        {
            0.0f,
            0.0f,
            0.0f
        };


        environment.specularMultiplier =
        {
            0.0f,
            0.0f,
            0.0f
        };


        // =========================================================
        // STAR
        // =========================================================

        SpaceSim::DirectionalLight sun;


        sun.direction =
            glm::normalize(
                glm::vec3(
                    -0.60f,
                     0.35f,
                    -0.70f));


        sun.radiance =
        {
            20.0f,
            19.5f,
            18.5f
        };


        // =========================================================
        // SHARED UNIT SPHERE MESH
        // =========================================================

        const SpaceSim::SphereMeshData sphereMeshData =
            SpaceSim::generateSphere(
                256,
                128);


        SpaceSim::GpuMesh sphereMesh(
            sphereMeshData.vertices,
            sphereMeshData.indices);


        // =========================================================
        // PLANET
        // =========================================================

        SpaceSim::RenderObject planet;


        planet.mesh =
            &sphereMesh;


        planet.modelMatrix =
            glm::translate(
                glm::mat4(1.0f),
                planetCenterWorld)
            *
            glm::scale(
                glm::mat4(1.0f),
                glm::vec3(
                    planetRadiusWorld));


        planet.material.baseColor =
        {
            0.10f,
            0.14f,
            0.18f
        };


        planet.material.metallic =
            0.0f;


        planet.material.roughness =
            0.90f;


        // =========================================================
        // MATTE LIGHTING BOX
        // =========================================================

        constexpr float boxWidthWorld =
            0.014f;


        constexpr float boxHeightWorld =
            0.006f;


        constexpr float boxLengthWorld =
            0.014f;


        const SpaceSim::BoxMeshData boxMeshData =
            SpaceSim::generateBox(
                boxWidthWorld,
                boxHeightWorld,
                boxLengthWorld);


        SpaceSim::GpuMesh boxMesh(
            boxMeshData.vertices,
            boxMeshData.indices);


        constexpr float boxZWorld =
            -0.080f;


        const float boxSurfaceY =
            std::sqrt(
                planetRadiusWorld *
                planetRadiusWorld
                -
                boxZWorld *
                boxZWorld);


        const glm::mat4 boxRotationY =
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(
                    -25.0f),
                glm::vec3(
                    0.0f,
                    1.0f,
                    0.0f));


        const glm::mat4 boxRotationX =
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(
                    12.0f),
                glm::vec3(
                    1.0f,
                    0.0f,
                    0.0f));


        SpaceSim::RenderObject referenceBox;


        referenceBox.mesh =
            &boxMesh;


        referenceBox.modelMatrix =
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(
                    -0.010f,

                    boxSurfaceY
                    +
                    boxHeightWorld *
                    0.5f
                    +
                    0.0025f,

                    boxZWorld))
            *
            boxRotationY
            *
            boxRotationX;


        referenceBox.material.baseColor =
        {
            0.50f,
            0.50f,
            0.50f
        };


        referenceBox.material.metallic =
            0.0f;


        referenceBox.material.roughness =
            0.80f;


        // =========================================================
        // METALLIC REFLECTION SPHERE
        // =========================================================
        //
        // This is our atmospheric-reflection reference object.
        //
        // It intentionally has:
        //
        // metallic = 1
        // low roughness
        //
        // so the atmospheric Sky-View should be clearly visible.

        constexpr float reflectionSphereX =
            0.025f;


        constexpr float reflectionSphereZ =
            -0.080f;


        constexpr float reflectionSphereRadius =
            0.006f;


        const float reflectionSphereSurfaceY =
            std::sqrt(
                planetRadiusWorld *
                planetRadiusWorld
                -
                reflectionSphereX *
                reflectionSphereX
                -
                reflectionSphereZ *
                reflectionSphereZ);


        SpaceSim::RenderObject reflectionSphere;


        reflectionSphere.mesh =
            &sphereMesh;


        reflectionSphere.modelMatrix =
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(
                    reflectionSphereX,

                    reflectionSphereSurfaceY
                    +
                    reflectionSphereRadius
                    +
                    0.0010f,

                    reflectionSphereZ))
            *
            glm::scale(
                glm::mat4(1.0f),
                glm::vec3(
                    reflectionSphereRadius));


        reflectionSphere.material.baseColor =
        {
            0.80f,
            0.80f,
            0.80f
        };


        reflectionSphere.material.metallic =
            1.0f;


        reflectionSphere.material.roughness =
            0.08f;


        // =========================================================
        // CAMERA
        // =========================================================

        SpaceSim::RenderCamera camera;


        camera.verticalFovDegrees =
            55.0f;


        camera.nearPlane =
            0.001f;


        camera.farPlane =
            1000.0f;


        int cameraMode =
            2;


        auto applyCameraMode =
            [&]()
            {
                if (cameraMode == 0)
                {
                    // =============================================
                    // SURFACE
                    // =============================================

                    const float altitudeKm =
                        1.0f;


                    camera.position =
                    {
                        0.0f,

                        planetRadiusWorld
                        +
                        altitudeKm *
                        worldUnitsPerKm,

                        0.0f
                    };


                    camera.forward =
                        glm::normalize(
                            glm::vec3(
                                0.0f,
                               -0.06f,
                               -1.0f));


                    camera.up =
                    {
                        0.0f,
                        1.0f,
                        0.0f
                    };


                    std::cout
                        << "\nCamera: SURFACE (~1 km)\n";
                }
                else if (cameraMode == 1)
                {
                    // =============================================
                    // UPPER ATMOSPHERE
                    // =============================================

                    const float altitudeKm =
                        40.0f;


                    camera.position =
                    {
                        0.0f,

                        planetRadiusWorld
                        +
                        altitudeKm *
                        worldUnitsPerKm,

                        0.0f
                    };


                    camera.forward =
                        glm::normalize(
                            glm::vec3(
                                0.0f,
                               -0.20f,
                               -1.0f));


                    camera.up =
                    {
                        0.0f,
                        1.0f,
                        0.0f
                    };


                    std::cout
                        << "\nCamera: UPPER ATMOSPHERE (~40 km)\n";
                }
                else
                {
                    // =============================================
                    // ORBIT
                    // =============================================

                    camera.position =
                    {
                        0.0f,
                        35.0f,
                        140.0f
                    };


                    camera.forward =
                        glm::normalize(
                            planetCenterWorld
                            -
                            camera.position);


                    camera.up =
                    {
                        0.0f,
                        1.0f,
                        0.0f
                    };


                    std::cout
                        << "\nCamera: ORBIT\n";
                }
            };


        applyCameraMode();


        // =========================================================
        // SCENE
        // =========================================================

        std::vector<SpaceSim::RenderObject> objects;


        objects.push_back(
            planet);


        objects.push_back(
            referenceBox);


        objects.push_back(
            reflectionSphere);


        // =========================================================
        // INPUT STATE
        // =========================================================

        bool key1WasDown =
            false;


        bool key2WasDown =
            false;


        bool key3WasDown =
            false;


        bool keyLWasDown =
            false;


        bool keyRWasDown =
            false;


        std::cout
            << "\nRendererLab controls:\n"
            << "  1 = surface camera\n"
            << "  2 = upper atmosphere camera\n"
            << "  3 = orbit camera\n"
            << "  L = atmospheric sunlight + diffuse fill\n"
            << "  R = atmospheric sky reflections\n\n";


        std::cout
            << "Atmospheric lighting: ON\n"
            << "Atmospheric reflections: ON\n";


        // =========================================================
        // MAIN LOOP
        // =========================================================

        while (window.processEvents())
        {
            const bool* keyboard =
                SDL_GetKeyboardState(
                    nullptr);


            const bool key1Down =
                keyboard[
                    SDL_SCANCODE_1];


            const bool key2Down =
                keyboard[
                    SDL_SCANCODE_2];


            const bool key3Down =
                keyboard[
                    SDL_SCANCODE_3];


            const bool keyLDown =
                keyboard[
                    SDL_SCANCODE_L];


            const bool keyRDown =
                keyboard[
                    SDL_SCANCODE_R];


            // =====================================================
            // CAMERA HOTKEYS
            // =====================================================

            if (key1Down &&
                !key1WasDown)
            {
                cameraMode =
                    0;


                applyCameraMode();
            }


            if (key2Down &&
                !key2WasDown)
            {
                cameraMode =
                    1;


                applyCameraMode();
            }


            if (key3Down &&
                !key3WasDown)
            {
                cameraMode =
                    2;


                applyCameraMode();
            }


            // =====================================================
            // DIFFUSE / DIRECT ATMOSPHERIC LIGHTING
            // =====================================================

            if (keyLDown &&
                !keyLWasDown)
            {
                atmosphericLightingEnabled =
                    !atmosphericLightingEnabled;


                renderer.setAtmosphereLightingEnabled(
                    atmosphericLightingEnabled);


                std::cout
                    << "Atmospheric lighting: "
                    << (
                        atmosphericLightingEnabled
                            ?
                            "ON"
                            :
                            "OFF"
                    )
                    << '\n';
            }


            // =====================================================
            // ATMOSPHERIC REFLECTIONS
            // =====================================================

            if (keyRDown &&
                !keyRWasDown)
            {
                atmosphericSpecularEnabled =
                    !atmosphericSpecularEnabled;


                renderer.setAtmosphereSpecularEnabled(
                    atmosphericSpecularEnabled);


                std::cout
                    << "Atmospheric reflections: "
                    << (
                        atmosphericSpecularEnabled
                            ?
                            "ON"
                            :
                            "OFF"
                    )
                    << '\n';
            }


            key1WasDown =
                key1Down;


            key2WasDown =
                key2Down;


            key3WasDown =
                key3Down;


            keyLWasDown =
                keyLDown;


            keyRWasDown =
                keyRDown;


            // =====================================================
            // WINDOW
            // =====================================================

            const int width =
                window.pixelWidth();


            const int height =
                window.pixelHeight();


            if (width <= 0 ||
                height <= 0)
            {
                SDL_Delay(
                    10);


                continue;
            }


            // =====================================================
            // RENDER
            // =====================================================

            renderer.render(
                width,
                height,
                camera,
                sun,
                environment,
                environmentMap,
                environmentIbl,
                objects,
                &atmosphereInstance);


            window.swapBuffers();
        }


        return
            0;
    }
    catch (const std::exception& exception)
    {
        std::cerr
            << "RendererLab fatal error: "
            << exception.what()
            << '\n';


        return
            1;
    }
}