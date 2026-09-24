#include "renderer/SceneRenderer.h"

#include "renderer/atmosphere/AtmosphereParameters.h"
#include "renderer/opengl/GpuMesh.h"

#include <glad/gl.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
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

          m_shadowMap(
              2048)
    {
    }

    void SceneRenderer::render(
        int width,
        int height,
        const RenderCamera &camera,
        const DirectionalLight &sun,
        const EnvironmentLight &environment,
        const GlTextureCube &environmentMap,
        const EnvironmentIbl &environmentIbl,
        const std::vector<RenderObject> &objects,
        const AtmosphereInstance *atmosphere)
    {
        static const std::vector<PlanetRenderObject>
            noPlanets;

        render(
            width,
            height,
            camera,
            sun,
            environment,
            environmentMap,
            environmentIbl,
            noPlanets,
            objects,
            atmosphere);
    }

    void SceneRenderer::render(
        int width,
        int height,
        const RenderCamera &camera,
        const DirectionalLight &sun,
        const EnvironmentLight &environment,
        const GlTextureCube &environmentMap,
        const EnvironmentIbl &environmentIbl,
        const std::vector<PlanetRenderObject> &planets,
        const std::vector<RenderObject> &objects,
        const AtmosphereInstance *atmosphere)
    {
        if (width <= 0 ||
            height <= 0)
        {
            return;
        }

        m_hdrTarget.resize(
            width,
            height);

        const float aspect =
            static_cast<float>(
                width) /
            static_cast<float>(
                height);

        const bool atmosphereActive =
            atmosphere != nullptr &&
            atmosphere->valid();

        const bool atmosphereLightingActive =
            atmosphereActive &&
            m_atmosphereLightingEnabled;

        const bool atmosphereSpecularActive =
            atmosphereActive &&
            m_atmosphereSpecularEnabled;

        // =========================================================
        // VIEW-DEPENDENT ATMOSPHERE
        // =========================================================

        if (atmosphereActive)
        {
            m_atmosphereViewLuts.update(
                camera,
                aspect,
                sun,
                *atmosphere);
        }

        // =========================================================
        // SHADOW CAMERA
        // =========================================================

        glm::vec3 sceneCenter(
            0.0f,
            0.0f,
            0.0f);

        float shadowExtent =
            10.0f;

        if (atmosphereActive)
        {
            sceneCenter =
                atmosphere->planetCenterWorld;

            shadowExtent =
                std::max(
                    10.0f,
                    atmosphere->planetRadiusWorld *
                        1.15f);
        }

        const float lightDistance =
            shadowExtent *
            3.0f;

        const glm::vec3 lightPosition =
            sceneCenter +
            sun.direction *
                lightDistance;

        glm::vec3 lightUp(
            0.0f,
            1.0f,
            0.0f);

        if (std::abs(
                glm::dot(
                    sun.direction,
                    lightUp)) >
            0.95f)
        {
            lightUp =
                {
                    0.0f,
                    0.0f,
                    1.0f};
        }

        const glm::mat4 lightView =
            glm::lookAt(
                lightPosition,
                sceneCenter,
                lightUp);

        const glm::mat4 lightProjection =
            glm::ortho(
                -shadowExtent,
                shadowExtent,
                -shadowExtent,
                shadowExtent,
                0.1f,
                lightDistance *
                    2.0f);

        const glm::mat4 lightSpaceMatrix =
            lightProjection *
            lightView;

        // =========================================================
        // PASS 1: LOCAL OBJECT SHADOW MAP
        // =========================================================
        //
        // The planet itself does not need to render into this map.
        //
        // Night-side planet occlusion is handled analytically by
        // the atmosphere/star direction logic.

        // Shadow maps retain the standard depth convention.
        glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glClearDepth(1.0);
        m_shadowMap.bindForWriting();

        glEnable(
            GL_DEPTH_TEST);

        m_shadowShader.use();

        m_shadowShader.setMat4(
            "lightSpaceMatrix",
            lightSpaceMatrix);

        for (const RenderObject &object :
             objects)
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
        // PASS 2: HDR SCENE
        // =========================================================

        // Reversed floating-point depth preserves nearby ships and distant planets.
        m_hdrTarget.bind();
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glDepthFunc(GL_GEQUAL);
        glDepthMask(GL_TRUE);
        glClearDepth(0.0);

        glViewport(
            0,
            0,
            width,
            height);

        const GLfloat background[4]{
            0.0f,
            0.0f,
            0.0f,
            1.0f};

        glClearBufferfv(
            GL_COLOR,
            0,
            background);

        const GLfloat noGeometry[4]{
            0.0f,
            0.0f,
            0.0f,
            0.0f};

        glClearBufferfv(
            GL_COLOR,
            1,
            noGeometry);

        glClear(
            GL_DEPTH_BUFFER_BIT);

        // =========================================================
        // PASS 2A: ENVIRONMENT
        // =========================================================

        if (!atmosphereActive)
        {
            m_environmentPass.render(
                camera,
                aspect,
                environmentMap);
        }

        // =========================================================
        // PASS 2B: STARFIELD / PRIMARY STAR
        // =========================================================

        m_starPass.render(
            camera,
            aspect,
            sun);

        // =========================================================
        // PASS 2C: PLANETS
        // =========================================================
        //
        // Planet now has its own dedicated material path.
        //
        // It still writes ordinary hardware depth and linear scene
        // depth, so everything after this works exactly as before.

        glEnable(
            GL_DEPTH_TEST);

        m_planetPass.render(
            camera,
            aspect,
            sun,
            planets,
            atmosphere,
            m_atmosphereViewLuts.skyReflectionTexture());

        // =========================================================
        // PASS 2D: GENERIC PBR OBJECTS
        // =========================================================

        m_pbrShader.use();

        m_pbrShader.setMat4(
            "view",
            camera.viewMatrix());

        m_pbrShader.setMat4(
            "projection",
            camera.projectionMatrix(
                aspect));

        m_pbrShader.setVec3(
            "cameraPosition",
            camera.position);

        m_pbrShader.setVec3(
            "sunDirection",
            sun.direction);

        m_pbrShader.setVec3(
            "sunRadiance",
            sun.radiance);

        m_pbrShader.setVec3(
            "environmentDiffuseMultiplier",
            environment.diffuseMultiplier);

        m_pbrShader.setVec3(
            "environmentSpecularMultiplier",
            environment.specularMultiplier);

        m_pbrShader.setMat4(
            "lightSpaceMatrix",
            lightSpaceMatrix);

        m_pbrShader.setInt(
            "shadowMap",
            1);

        m_pbrShader.setInt(
            "irradianceMap",
            3);

        m_pbrShader.setInt(
            "prefilteredEnvironmentMap",
            4);

        m_pbrShader.setInt(
            "brdfLut",
            5);

        glBindTextureUnit(
            1,
            m_shadowMap.depthTexture());

        glBindTextureUnit(
            3,
            environmentIbl
                .irradianceMap()
                .id());

        glBindTextureUnit(
            4,
            environmentIbl
                .prefilteredMap()
                .id());

        glBindTextureUnit(
            5,
            environmentIbl
                .brdfLut()
                .id());

        // =========================================================
        // ATMOSPHERIC PBR INPUTS
        // =========================================================

        m_pbrShader.setInt(
            "atmosphereTransmittanceLut",
            7);

        m_pbrShader.setInt(
            "atmosphereSkyIrradianceLut",
            8);

        m_pbrShader.setInt(
            "atmosphereSkyViewLut",
            9);

        m_pbrShader.setInt(
            "atmosphereLightingEnabled",
            atmosphereLightingActive
                ? 1
                : 0);

        m_pbrShader.setInt(
            "atmosphereSpecularEnabled",
            atmosphereSpecularActive
                ? 1
                : 0);

        if (atmosphereActive)
        {
            const AtmosphereParameters &parameters =
                *atmosphere->parameters;

            const float kmPerWorldUnit =
                parameters.bottomRadiusKm /
                atmosphere->planetRadiusWorld;

            const float atmosphereThicknessWorld =
                (parameters.topRadiusKm -
                 parameters.bottomRadiusKm) /
                kmPerWorldUnit;

            m_pbrShader.setVec3(
                "atmospherePlanetCenterWorld",
                atmosphere->planetCenterWorld);

            m_pbrShader.setFloat(
                "atmosphereKmPerWorldUnit",
                kmPerWorldUnit);

            m_pbrShader.setFloat(
                "atmosphereBottomRadiusKm",
                parameters.bottomRadiusKm);

            m_pbrShader.setFloat(
                "atmosphereTopRadiusKm",
                parameters.topRadiusKm);

            m_pbrShader.setVec3(
                "atmosphereGroundAlbedo",
                parameters.groundAlbedo);

            m_pbrShader.setFloat(
                "atmosphereSpecularProbeRangeWorld",
                atmosphereThicknessWorld);

            glBindTextureUnit(
                7,
                atmosphere
                    ->luts
                    ->transmittance()
                    .id());

            glBindTextureUnit(
                8,
                atmosphere
                    ->luts
                    ->skyIrradiance()
                    .id());

            glBindTextureUnit(
                9,
                m_atmosphereViewLuts
                    .skyReflectionTexture());
        }

        for (const RenderObject &object :
             objects)
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

            m_pbrShader.setVec3(
                "emissiveColor",
                object.material.emissiveColor);

            m_pbrShader.setFloat(
                "emissiveStrength",
                object.material.emissiveStrength);

            object.mesh->draw();
        }

        // =========================================================
        // PASS 3: ATMOSPHERE
        glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        glDepthFunc(GL_LESS);
        glClearDepth(1.0);
        // =========================================================

        GLuint finalHdrTexture =
            m_hdrTarget.colorTexture();

        if (atmosphereActive)
        {
            finalHdrTexture =
                m_atmospherePass.render(
                    width,
                    height,
                    m_hdrTarget.colorTexture(),
                    m_hdrTarget.linearDepthTexture(),
                    camera,
                    aspect,
                    sun,
                    *atmosphere,
                    m_atmosphereViewLuts);
        }
        // Temporary debug composite; physical atmosphere/cloud coupling comes next.
        if (atmosphereActive && atmosphere->cloudsValid())
            finalHdrTexture = m_cloudPass.render(width, height, finalHdrTexture, m_hdrTarget.linearDepthTexture(), camera, aspect, sun, *atmosphere); // =========================================================
        // PASS 4: AUTO EXPOSURE
        // =========================================================

        if (m_autoExposureEnabled)
        {
            m_autoExposurePass.update(
                finalHdrTexture,
                width,
                height);
        }

        // =========================================================
        // PASS 5: BLOOM
        // =========================================================

        const GLuint bloomTexture =
            m_bloomPass.render(
                finalHdrTexture,
                width,
                height);

        // =========================================================
        // PASS 6: TONEMAP
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
            finalHdrTexture,
            bloomTexture,
            m_autoExposurePass
                .exposureTexture(),
            m_autoExposureEnabled,
            m_exposureCompensation,
            m_bloomStrength);
    }
}