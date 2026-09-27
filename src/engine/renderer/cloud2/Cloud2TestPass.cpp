#include "renderer/cloud2/Cloud2TestPass.h"

#include "renderer/CameraRayReconstruction.h"

#include "renderer/atmosphere/AtmosphereParameters.h"

#include <glm/geometric.hpp>

#include <iostream>
#include <stdexcept>


namespace SpaceSim
{
    Cloud2TestPass::Cloud2TestPass()
        : m_shader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/cloud2/cloud2_test.frag")
    {
        glCreateVertexArrays(
            1,
            &m_vertexArray);


        m_shader.setInt(
            "sceneColorTexture",
            0);


        m_shader.setInt(
            "sceneLinearDepthTexture",
            1);
    }


    Cloud2TestPass::~Cloud2TestPass()
    {
        destroyTarget();


        if (m_vertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vertexArray);


            m_vertexArray =
                0;
        }
    }


    void Cloud2TestPass::destroyTarget()
    {
        if (m_colorTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_colorTexture);


            m_colorTexture =
                0;
        }


        if (m_framebuffer != 0)
        {
            glDeleteFramebuffers(
                1,
                &m_framebuffer);


            m_framebuffer =
                0;
        }


        m_width =
            0;


        m_height =
            0;
    }


    void Cloud2TestPass::resize(
        int width,
        int height)
    {
        if (width <= 0 ||
            height <= 0)
        {
            return;
        }


        if (width == m_width &&
            height == m_height)
        {
            return;
        }


        destroyTarget();


        m_width =
            width;


        m_height =
            height;


        glCreateFramebuffers(
            1,
            &m_framebuffer);


        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_colorTexture);


        glTextureStorage2D(
            m_colorTexture,
            1,
            GL_RGBA16F,
            width,
            height);


        glTextureParameteri(
            m_colorTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_colorTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_colorTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_colorTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glNamedFramebufferTexture(
            m_framebuffer,
            GL_COLOR_ATTACHMENT0,
            m_colorTexture,
            0);


        glNamedFramebufferDrawBuffer(
            m_framebuffer,
            GL_COLOR_ATTACHMENT0);


        const GLenum status =
            glCheckNamedFramebufferStatus(
                m_framebuffer,
                GL_FRAMEBUFFER);


        if (status !=
            GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error(
                "Cloud2 framebuffer is incomplete.");
        }
    }


    void Cloud2TestPass::lockFormationIfNeeded(
        const RenderCamera& camera,
        const AtmosphereInstance& atmosphere)
    {
        if (m_formationLocked ||
            !atmosphere.valid())
        {
            return;
        }


        const AtmosphereParameters& parameters =
            *atmosphere.parameters;


        if (atmosphere.planetRadiusWorld <=
            0.0f)
        {
            return;
        }


        const float kmPerWorldUnit =
            parameters.bottomRadiusKm /
            atmosphere.planetRadiusWorld;


        const glm::vec3 cameraFromPlanetKm =
            (
                camera.position -
                atmosphere.planetCenterWorld
            )
            *
            kmPerWorldUnit;


        const float cameraRadiusKm =
            glm::length(
                cameraFromPlanetKm);


        if (cameraRadiusKm <=
            0.001f)
        {
            return;
        }


        const float altitudeKm =
            cameraRadiusKm -
            parameters.bottomRadiusKm;


        // =====================================================
        // WAIT UNTIL WE HAVE ACTUALLY ARRIVED AT THE PLANET
        // =====================================================
        //
        // Hyperdrive currently exits about 1 km above the ocean.
        //
        // We don't want Cloud2 choosing its location while the
        // player is still hundreds/thousands of kilometres away.
        //
        // Once the camera drops below 5 km for the first time,
        // we consider that our test arrival.
        // =====================================================

        if (altitudeKm >
            5.0f)
        {
            return;
        }


        // =====================================================
        // PUT THE TEST CLOUD DIRECTLY ABOVE THE PLAYER
        // =====================================================
        //
        // The radial direction from the planet center through the
        // player's arrival position becomes the permanent cloud
        // location.
        //
        // At the current hyperdrive arrival:
        //
        //     player altitude ≈ 1.0 km
        //     cloud base      = 1.5 km
        //
        // so the player starts about 500 m directly underneath
        // the cloud.
        //
        // IMPORTANT:
        //
        // We lock this direction ONCE.
        //
        // The cloud does NOT continue following the player after
        // this point.
        // =====================================================

        m_formation.planetDirection =
            glm::normalize(
                cameraFromPlanetKm);


        m_formationLocked =
            true;


        std::cout
            << "\n[Cloud2] Test cumulus locked directly above arrival point.\n"
            << "[Cloud2] Camera altitude: "
            << altitudeKm
            << " km\n"
            << "[Cloud2] Cloud base: "
            << m_formation.baseAltitudeKm
            << " km\n"
            << "[Cloud2] Cloud top: "
            << (
                   m_formation.baseAltitudeKm +
                   m_formation.heightKm
               )
            << " km\n"
            << "[Cloud2] Horizontal radius: "
            << m_formation.horizontalRadiusKm
            << " km\n"
            << "[Cloud2] Distance below base at lock: "
            << (
                   m_formation.baseAltitudeKm -
                   altitudeKm
               )
            << " km\n\n";
    }


    GLuint Cloud2TestPass::render(
        int width,
        int height,
        GLuint sceneColorTexture,
        GLuint sceneLinearDepthTexture,
        const RenderCamera& camera,
        float aspectRatio,
        const DirectionalLight& sun,
        const AtmosphereInstance* atmosphere)
    {
        if (width <= 0 ||
            height <= 0 ||
            atmosphere == nullptr ||
            !atmosphere->valid())
        {
            return
                sceneColorTexture;
        }


        lockFormationIfNeeded(
            camera,
            *atmosphere);


        if (!m_formationLocked ||
            !m_formation.valid())
        {
            return
                sceneColorTexture;
        }


        resize(
            width,
            height);


        const AtmosphereParameters& parameters =
            *atmosphere->parameters;


        const float kmPerWorldUnit =
            parameters.bottomRadiusKm /
            atmosphere->planetRadiusWorld;


        const glm::mat4 rayReconstructionMatrix =
            makeCameraRayReconstructionMatrix(
                camera,
                aspectRatio);


        // =====================================================
        // TARGET
        // =====================================================

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_framebuffer);


        glViewport(
            0,
            0,
            width,
            height);


        glDisable(
            GL_DEPTH_TEST);


        // =====================================================
        // SHADER INPUTS
        // =====================================================

        m_shader.use();


        m_shader.setMat4(
            "inverseViewProjection",
            rayReconstructionMatrix);


        m_shader.setVec3(
            "cameraPositionWorld",
            camera.position);


        m_shader.setVec3(
            "planetCenterWorld",
            atmosphere->planetCenterWorld);


        m_shader.setFloat(
            "kmPerWorldUnit",
            kmPerWorldUnit);


        m_shader.setFloat(
            "planetRadiusKm",
            parameters.bottomRadiusKm);


        m_shader.setVec3(
            "formationDirection",
            glm::normalize(
                m_formation.planetDirection));


        m_shader.setFloat(
            "formationBaseAltitudeKm",
            m_formation.baseAltitudeKm);


        m_shader.setFloat(
            "formationHorizontalRadiusKm",
            m_formation.horizontalRadiusKm);


        m_shader.setFloat(
            "formationHeightKm",
            m_formation.heightKm);


        m_shader.setFloat(
            "formationSeed",
            m_formation.seed);


        m_shader.setFloat(
            "formationDensityMultiplier",
            m_formation.densityMultiplier);


        m_shader.setFloat(
            "cloudExtinctionPerKm",
            m_formation.extinctionPerKm);


        m_shader.setVec3(
            "sunDirection",
            glm::normalize(
                sun.direction));


        m_shader.setVec3(
            "sunRadiance",
            sun.radiance);


        glBindTextureUnit(
            0,
            sceneColorTexture);


        glBindTextureUnit(
            1,
            sceneLinearDepthTexture);


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


        return
            m_colorTexture;
    }
}