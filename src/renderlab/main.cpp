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
#include <glm/vec4.hpp>

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
            atmosphere.bottomRadiusKm /
            planetRadiusWorld;


        const float worldUnitsPerKm =
            1.0f /
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
            1.0f);


        renderer.setBloomStrength(
            0.045f);

        renderer.setBloomThreshold(
            8.0f);
            
        bool atmosphericLightingEnabled =
            true;


        bool atmosphericSpecularEnabled =
            true;


        renderer.setAtmosphereLightingEnabled(
            atmosphericLightingEnabled);


        renderer.setAtmosphereSpecularEnabled(
            atmosphericSpecularEnabled);


        // =========================================================
        // TEST ENVIRONMENT / OLD IBL
        // =========================================================

        SpaceSim::GlTextureCube environmentMap(
            64);


        SpaceSim::fillTestEnvironment(
            environmentMap);


        SpaceSim::EnvironmentIbl environmentIbl(
            environmentMap);


        SpaceSim::EnvironmentLight environment;


        // Disable the old artificial environment contribution while
        // we're testing the physical atmosphere.

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
        // PRIMARY STAR
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


        // DirectionalLight now also contains the visible stellar
        // disk properties added in the previous step.
        //
        // We're leaving the default Sun-like apparent size for now.


        // =========================================================
        // SHARED SPHERE MESH
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
        // MATTE LIGHTING REFERENCE BOX
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
            -0.085f;


        constexpr float boxXWorld =
            -0.020f;


        const float boxSurfaceY =
            std::sqrt(
                planetRadiusWorld *
                planetRadiusWorld
                -
                boxXWorld *
                boxXWorld
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
                    boxXWorld,

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
        // METALLIC MATERIAL TEST SPHERES
        // =========================================================

        constexpr float reflectionSphereRadius =
            0.0048f;


        constexpr float reflectionSphereZ =
            -0.090f;


        auto makeReflectionSphere =
            [&](float xPosition,
                float materialRoughness)
            {
                const float surfaceY =
                    std::sqrt(
                        planetRadiusWorld *
                        planetRadiusWorld
                        -
                        xPosition *
                        xPosition
                        -
                        reflectionSphereZ *
                        reflectionSphereZ);


                SpaceSim::RenderObject sphere;


                sphere.mesh =
                    &sphereMesh;


                sphere.modelMatrix =
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(
                            xPosition,

                            surfaceY
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


                sphere.material.baseColor =
                {
                    0.80f,
                    0.80f,
                    0.80f
                };


                sphere.material.metallic =
                    1.0f;


                sphere.material.roughness =
                    materialRoughness;


                return
                    sphere;
            };


        SpaceSim::RenderObject polishedSphere =
            makeReflectionSphere(
                0.008f,
                0.05f);


        SpaceSim::RenderObject mediumSphere =
            makeReflectionSphere(
                0.027f,
                0.30f);


        SpaceSim::RenderObject roughSphere =
            makeReflectionSphere(
                0.046f,
                0.70f);


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


        // =========================================================
        // FREE-LOOK CAMERA SETTINGS
        // =========================================================
        //
        // Hold RMB and move the mouse.
        //
        // The value here is radians per mouse pixel.
        //
        // 0.0025 rad ~= 0.14 degrees per pixel.

        constexpr float mouseLookSensitivity =
            0.0025f;


        auto rotateDirectionAroundAxis =
            [](
                const glm::vec3& direction,
                float angleRadians,
                const glm::vec3& axis)
            {
                const glm::mat4 rotation =
                    glm::rotate(
                        glm::mat4(1.0f),
                        angleRadians,
                        glm::normalize(
                            axis));


                const glm::vec4 rotated =
                    rotation
                    *
                    glm::vec4(
                        direction,
                        0.0f);


                return
                    glm::normalize(
                        glm::vec3(
                            rotated));
            };


        // =========================================================
        // CAMERA PRESETS
        // =========================================================
        //
        // 1 = near surface
        // 2 = upper atmosphere
        // 3 = orbit
        //
        // The preset resets both position and viewing direction.
        //
        // After selecting one, RMB free-look can rotate from there.

        int cameraMode =
            2;


        auto applyCameraMode =
            [&]()
            {
                if (cameraMode == 0)
                {
                    // =============================================
                    // SURFACE — ~1 km altitude
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
                    // UPPER ATMOSPHERE — ~40 km
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
            polishedSphere);


        objects.push_back(
            mediumSphere);


        objects.push_back(
            roughSphere);


        // =========================================================
        // INPUT STATE
        // =========================================================

        bool key1WasDown =
            false;


        bool key2WasDown =
            false;


        bool key3WasDown =
            false;


        bool key4WasDown =
            false;


        bool keyLWasDown =
            false;


        bool keyRWasDown =
            false;


        bool rightMouseWasDown =
            false;


        // =========================================================
        // CONTROLS
        // =========================================================

        std::cout
            << "\nRendererLab controls:\n"
            << "  1 = surface camera\n"
            << "  2 = upper atmosphere camera\n"
            << "  3 = orbit camera\n"
            << "  4 = look directly at primary star\n"
            << "  Hold RMB + mouse = free look\n"
            << "  L = atmospheric sunlight + diffuse fill\n"
            << "  R = atmospheric reflections\n\n";


        std::cout
            << "Material spheres:\n"
            << "  left   roughness = 0.05\n"
            << "  middle roughness = 0.30\n"
            << "  right  roughness = 0.70\n\n";


        std::cout
            << "Atmospheric lighting: ON\n"
            << "Atmospheric reflections: ON\n";


        // =========================================================
        // MAIN LOOP
        // =========================================================

        while (window.processEvents())
        {
            // =====================================================
            // KEYBOARD INPUT
            // =====================================================

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


            const bool key4Down =
                keyboard[
                    SDL_SCANCODE_4];


            const bool keyLDown =
                keyboard[
                    SDL_SCANCODE_L];


            const bool keyRDown =
                keyboard[
                    SDL_SCANCODE_R];


            // =====================================================
            // CAMERA PRESETS
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
            // LOOK DIRECTLY AT STAR
            // =====================================================
            //
            // This doesn't move the camera.
            //
            // It only rotates it so the primary star should appear
            // in the exact center of the screen.
            //
            // This is useful for distinguishing:
            //
            //     "star is off-screen"
            //
            // from:
            //
            //     "StarPass isn't rendering."

            if (key4Down &&
                !key4WasDown)
            {
                camera.forward =
                    glm::normalize(
                        sun.direction);


                std::cout
                    << "Camera: looking directly at primary star\n";
            }


            // =====================================================
            // ATMOSPHERIC DIRECT + DIFFUSE LIGHTING
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
            // ATMOSPHERIC SPECULAR REFLECTIONS
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


            // =====================================================
            // RIGHT-MOUSE FREE LOOK
            // =====================================================

            const SDL_MouseButtonFlags mouseButtons =
                SDL_GetMouseState(
                    nullptr,
                    nullptr);


            const bool rightMouseDown =
                (
                    mouseButtons
                    &
                    SDL_BUTTON_RMASK
                )
                !=
                0;


            // Enter relative mouse mode when RMB is first pressed.
            //
            // SDL then hides/confines the cursor and gives us
            // continuous relative movement instead of absolute
            // cursor coordinates.

            if (rightMouseDown &&
                !rightMouseWasDown)
            {
                SDL_Window* mouseWindow =
                    SDL_GetMouseFocus();


                if (mouseWindow)
                {
                    if (!SDL_SetWindowRelativeMouseMode(
                            mouseWindow,
                            true))
                    {
                        std::cerr
                            << "Could not enable relative mouse mode: "
                            << SDL_GetError()
                            << '\n';
                    }
                }
            }


            // Return the cursor to normal when RMB is released.

            if (!rightMouseDown &&
                rightMouseWasDown)
            {
                SDL_Window* mouseWindow =
                    SDL_GetMouseFocus();


                if (mouseWindow)
                {
                    if (!SDL_SetWindowRelativeMouseMode(
                            mouseWindow,
                            false))
                    {
                        std::cerr
                            << "Could not disable relative mouse mode: "
                            << SDL_GetError()
                            << '\n';
                    }
                }
            }


            float mouseDeltaX =
                0.0f;


            float mouseDeltaY =
                0.0f;


            SDL_GetRelativeMouseState(
                &mouseDeltaX,
                &mouseDeltaY);


            if (rightMouseDown)
            {
                // -------------------------------------------------
                // YAW
                // -------------------------------------------------
                //
                // Moving the mouse horizontally rotates around the
                // camera's current up axis.

                const float yawRadians =
                    -mouseDeltaX *
                    mouseLookSensitivity;


                camera.forward =
                    rotateDirectionAroundAxis(
                        camera.forward,
                        yawRadians,
                        camera.up);


                // -------------------------------------------------
                // PITCH
                // -------------------------------------------------

                const glm::vec3 right =
                    glm::normalize(
                        glm::cross(
                            camera.forward,
                            camera.up));


                const float pitchRadians =
                    -mouseDeltaY *
                    mouseLookSensitivity;


                const glm::vec3 pitchedForward =
                    rotateDirectionAroundAxis(
                        camera.forward,
                        pitchRadians,
                        right);


                // Prevent the camera from flipping upside down when
                // looking almost exactly parallel to its up axis.

                const float upAlignment =
                    std::abs(
                        glm::dot(
                            pitchedForward,
                            glm::normalize(
                                camera.up)));


                if (upAlignment <
                    0.995f)
                {
                    camera.forward =
                        pitchedForward;
                }
            }


            // =====================================================
            // STORE PREVIOUS INPUT STATE
            // =====================================================

            key1WasDown =
                key1Down;


            key2WasDown =
                key2Down;


            key3WasDown =
                key3Down;


            key4WasDown =
                key4Down;


            keyLWasDown =
                keyLDown;


            keyRWasDown =
                keyRDown;


            rightMouseWasDown =
                rightMouseDown;


            // =====================================================
            // WINDOW SIZE
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