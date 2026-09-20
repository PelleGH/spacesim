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

#include "renderer/planet/PlanetRenderObject.h"

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
#include <fstream>
#include <string>
#include <glad/gl.h>

int main(int argc, char** argv)
{
    const bool oceanCheck=argc>1 && std::string(argv[1])=="--ocean-check";
    int checkFrame=0;
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
                0.18f};

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

        const glm::vec3 planetCenterWorld{
            0.0f,
            0.0f,
            0.0f};

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

        // Auto exposure now controls the main exposure.
        //
        // This value is only a manual multiplier on top.
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
        //
        // These remain available for the generic PBR renderer,
        // although the old environment contribution is disabled
        // while testing the physical atmosphere.

        SpaceSim::GlTextureCube environmentMap(
            64);

        SpaceSim::fillTestEnvironment(
            environmentMap);

        SpaceSim::EnvironmentIbl environmentIbl(
            environmentMap);

        SpaceSim::EnvironmentLight environment;

        environment.diffuseMultiplier =
            {
                0.0f,
                0.0f,
                0.0f};

        environment.specularMultiplier =
            {
                0.0f,
                0.0f,
                0.0f};

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
                18.5f};

        // DirectionalLight also contains the visible stellar-disk
        // parameters.
        //
        // Defaults currently give us a Sun-like apparent disk.

        // =========================================================
        // SHARED SPHERE MESH
        // =========================================================
        //
        // The procedural planet and all three material-test spheres
        // share this geometry.

        const SpaceSim::SphereMeshData sphereMeshData =
            SpaceSim::generateSphere(
                1024,
                512);

        SpaceSim::GpuMesh sphereMesh(
            sphereMeshData.vertices,
            sphereMeshData.indices);

        // =========================================================
        // DEDICATED PLANET
        // =========================================================
        //
        // The planet no longer goes through generic RenderObject.
        //
        // Planet-specific material data now goes through PlanetPass.

        SpaceSim::PlanetRenderObject planet;

        planet.hasOcean = true;
        planet.radiusKm = atmosphere.bottomRadiusKm;

        planet.mesh =
            &sphereMesh;

        planet.modelMatrix =
            glm::translate(
                glm::mat4(1.0f),
                planetCenterWorld) *
            glm::scale(
                glm::mat4(1.0f),
                glm::vec3(
                    planetRadiusWorld));

        // =========================================================
        // OCEAN MATERIAL
        // =========================================================

        planet.material.deepOceanColor =
            {
                0.006f,
                0.022f,
                0.055f};

        planet.material.shallowOceanColor =
            {
                0.020f,
                0.100f,
                0.145f};

        planet.material.oceanRoughness =
            0.10f;

        // =========================================================
        // LAND MATERIAL
        // =========================================================

        planet.material.lowLandColor =
            {
                0.08f,
                0.20f,
                0.055f};

        planet.material.highLandColor =
            {
                0.38f,
                0.30f,
                0.17f};

        planet.material.landRoughness =
            0.82f;

        // =========================================================
        // PROCEDURAL SURFACE PARAMETERS
        // =========================================================

        planet.material.continentScale =
            2.4f;

        planet.material.detailScale =
            10.0f;

        planet.material.oceanLevel =
            0.56f;

        planet.material.coastWidth =
            0.018f;

        planet.material.seed =
            13.37f;

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
                    planetRadiusWorld -
                boxXWorld *
                    boxXWorld -
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

                    boxSurfaceY +
                        boxHeightWorld *
                            0.5f +
                        0.0025f,

                    boxZWorld)) *
            boxRotationY *
            boxRotationX;

        referenceBox.material.baseColor =
            {
                0.50f,
                0.50f,
                0.50f};

        referenceBox.material.metallic =
            0.0f;

        referenceBox.material.roughness =
            0.80f;

        // =========================================================
        // METALLIC MATERIAL TEST SPHERES
        // =========================================================
        //
        // All three have identical material properties except
        // roughness:
        //
        //     0.05
        //     0.30
        //     0.70

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
                        planetRadiusWorld -
                    xPosition *
                        xPosition -
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

                        surfaceY +
                            reflectionSphereRadius +
                            0.0010f,

                        reflectionSphereZ)) *
                glm::scale(
                    glm::mat4(1.0f),
                    glm::vec3(
                        reflectionSphereRadius));

            sphere.material.baseColor =
                {
                    0.80f,
                    0.80f,
                    0.80f};

            sphere.material.metallic =
                1.0f;

            sphere.material.roughness =
                materialRoughness;

            return sphere;
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
        // FREE-LOOK CAMERA
        // =========================================================

        constexpr float mouseLookSensitivity =
            0.0025f;

        auto rotateDirectionAroundAxis =
            [](
                const glm::vec3 &direction,
                float angleRadians,
                const glm::vec3 &axis)
        {
            const glm::mat4 rotation =
                glm::rotate(
                    glm::mat4(1.0f),
                    angleRadians,
                    glm::normalize(
                        axis));

            const glm::vec4 rotated =
                rotation *
                glm::vec4(
                    direction,
                    0.0f);

            return glm::normalize(
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
        // 4 = point directly toward primary star

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
                    0.05f;

                camera.position =
                    {
                        0.0f,

                        planetRadiusWorld +
                            altitudeKm *
                                worldUnitsPerKm,

                        0.0f};

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
                        0.0f};

                std::cout
                    << "\nCamera: SURFACE (~50 m)\n";
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

                        planetRadiusWorld +
                            altitudeKm *
                                worldUnitsPerKm,

                        0.0f};

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
                        0.0f};

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
                        140.0f};

                camera.forward =
                    glm::normalize(
                        planetCenterWorld -
                        camera.position);

                camera.up =
                    {
                        0.0f,
                        1.0f,
                        0.0f};

                std::cout
                    << "\nCamera: ORBIT\n";
            }
        };

        applyCameraMode();

        // =========================================================
        // SCENE
        // =========================================================

        std::vector<SpaceSim::PlanetRenderObject> planets;

        planets.push_back(
            planet);

        std::vector<SpaceSim::RenderObject> objects;

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
            << "Planet material:\n"
            << "  dedicated procedural land/ocean pass\n\n";

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

            const bool *keyboard =
                SDL_GetKeyboardState(
                    nullptr);

            const bool key1Down =
                keyboard[SDL_SCANCODE_1];

            const bool key2Down =
                keyboard[SDL_SCANCODE_2];

            const bool key3Down =
                keyboard[SDL_SCANCODE_3];

            const bool key4Down =
                keyboard[SDL_SCANCODE_4];

            const bool keyLDown =
                keyboard[SDL_SCANCODE_L];

            const bool keyRDown =
                keyboard[SDL_SCANCODE_R];

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
                    << (atmosphericLightingEnabled
                            ? "ON"
                            : "OFF")
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
                    << (atmosphericSpecularEnabled
                            ? "ON"
                            : "OFF")
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
                (mouseButtons &
                 SDL_BUTTON_RMASK) !=
                0;

            if (rightMouseDown &&
                !rightMouseWasDown)
            {
                SDL_Window *mouseWindow =
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

            if (!rightMouseDown &&
                rightMouseWasDown)
            {
                SDL_Window *mouseWindow =
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

                // Prevent the free camera from flipping upside down.

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
            // STORE INPUT STATE
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

            if(oceanCheck) {
                const int shot=checkFrame/24;
                const float altitudeKm=shot==2?1.0f:shot==3?3.9f:0.05f;
                const glm::vec3 up=glm::normalize(glm::vec3(shot==1?.000024f:0,1,0));
                camera.position=planetCenterWorld+up*(planetRadiusWorld+altitudeKm*worldUnitsPerKm);
                camera.forward=glm::normalize(glm::vec3(0,-.06f,-1)); camera.up=up;
                if(shot==5) { camera.position={0,35,140}; camera.forward=glm::normalize(-camera.position); camera.up={0,1,0}; }
            }
            // The previous near plane was ~127 m: it clipped the nearest waves
            // in the 50 m surface preset. Keep metre-scale clearance nearby.
            const float altitudeWorld=glm::length(camera.position-planetCenterWorld)-planetRadiusWorld;
            camera.nearPlane=glm::clamp(altitudeWorld*.025f,.00001f,.01f);
            renderer.render(
                width,
                height,
                camera,
                sun,
                environment,
                environmentMap,
                environmentIbl,
                planets,
                objects,
                oceanCheck && checkFrame/24==4 ? nullptr : &atmosphereInstance);

            if(oceanCheck) {
                if(checkFrame%24==23) {
                    std::vector<unsigned char> pixels(width*height*3);
                    glPixelStorei(GL_PACK_ALIGNMENT,1);
                    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
                    std::ofstream image("ocean-check-"+std::to_string(checkFrame/24)+".ppm",std::ios::binary);
                    image<<"P6\n"<<width<<" "<<height<<"\n255\n";
                    for(int row=height-1;row>=0;--row) image.write(reinterpret_cast<const char*>(pixels.data()+row*width*3),width*3);
                    const GLenum error=glGetError();
                    std::cout<<"Ocean check shot "<<checkFrame/24<<", GL error "<<error<<std::endl;
                    if(error!=GL_NO_ERROR) return 2;
                }
                ++checkFrame;
                if(checkFrame>=144) break;
            }
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