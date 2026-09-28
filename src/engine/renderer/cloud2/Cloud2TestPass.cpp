#include "renderer/cloud2/Cloud2TestPass.h"

#include "renderer/CameraRayReconstruction.h"

#include "renderer/atmosphere/AtmosphereParameters.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>


namespace SpaceSim
{
    Cloud2TestPass::Cloud2TestPass()
        : m_volumeShader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/cloud2/cloud2_test.frag"),

          m_spatialShader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/cloud2/cloud2_spatial.frag"),

          m_compositeShader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/cloud2/cloud2_composite.frag")
    {
        glCreateVertexArrays(
            1,
            &m_vertexArray);


        // =====================================================
        // VOLUME PASS
        // =====================================================

        m_volumeShader.setInt(
            "sceneLinearDepthTexture",
            0);


        m_volumeShader.setInt(
            "cloudDensityVolume",
            1);


        // =====================================================
        // SPATIAL PASS
        // =====================================================

        m_spatialShader.setInt(
            "rawCloudTexture",
            0);


        m_spatialShader.setInt(
            "rawCloudDepthTexture",
            1);


        // =====================================================
        // COMPOSITE PASS
        // =====================================================

        m_compositeShader.setInt(
            "sceneColorTexture",
            0);


        m_compositeShader.setInt(
            "cloudVolumeTexture",
            1);


        m_compositeShader.setInt(
            "sceneLinearDepthTexture",
            2);


        m_compositeShader.setInt(
            "cloudDepthTexture",
            3);
    }


    Cloud2TestPass::~Cloud2TestPass()
    {
        destroyTargets();


        if (m_vertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vertexArray);


            m_vertexArray =
                0;
        }
    }


    void Cloud2TestPass::destroyTargets()
    {
        if (m_volumeCloudTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_volumeCloudTexture);


            m_volumeCloudTexture =
                0;
        }


        if (m_volumeDepthTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_volumeDepthTexture);


            m_volumeDepthTexture =
                0;
        }


        if (m_volumeFramebuffer != 0)
        {
            glDeleteFramebuffers(
                1,
                &m_volumeFramebuffer);


            m_volumeFramebuffer =
                0;
        }


        if (m_spatialCloudTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_spatialCloudTexture);


            m_spatialCloudTexture =
                0;
        }


        if (m_spatialDepthTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_spatialDepthTexture);


            m_spatialDepthTexture =
                0;
        }


        if (m_spatialFramebuffer != 0)
        {
            glDeleteFramebuffers(
                1,
                &m_spatialFramebuffer);


            m_spatialFramebuffer =
                0;
        }


        if (m_compositeTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_compositeTexture);


            m_compositeTexture =
                0;
        }


        if (m_compositeFramebuffer != 0)
        {
            glDeleteFramebuffers(
                1,
                &m_compositeFramebuffer);


            m_compositeFramebuffer =
                0;
        }


        m_width =
            0;


        m_height =
            0;


        m_volumeWidth =
            0;


        m_volumeHeight =
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


        destroyTargets();


        m_width =
            width;


        m_height =
            height;


        m_volumeWidth =
            std::max(
                1,
                (width + 1) /
                    2);


        m_volumeHeight =
            std::max(
                1,
                (height + 1) /
                    2);


        // =====================================================
        // RAW HALF-RES CLOUD
        // =====================================================

        glCreateFramebuffers(
            1,
            &m_volumeFramebuffer);


        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_volumeCloudTexture);


        glTextureStorage2D(
            m_volumeCloudTexture,
            1,
            GL_RGBA16F,
            m_volumeWidth,
            m_volumeHeight);


        glTextureParameteri(
            m_volumeCloudTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_volumeCloudTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_volumeCloudTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_volumeCloudTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_volumeDepthTexture);


        glTextureStorage2D(
            m_volumeDepthTexture,
            1,
            GL_R32F,
            m_volumeWidth,
            m_volumeHeight);


        glTextureParameteri(
            m_volumeDepthTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);


        glTextureParameteri(
            m_volumeDepthTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);


        glTextureParameteri(
            m_volumeDepthTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_volumeDepthTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glNamedFramebufferTexture(
            m_volumeFramebuffer,
            GL_COLOR_ATTACHMENT0,
            m_volumeCloudTexture,
            0);


        glNamedFramebufferTexture(
            m_volumeFramebuffer,
            GL_COLOR_ATTACHMENT1,
            m_volumeDepthTexture,
            0);


        const GLenum volumeDrawBuffers[2]
        {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1
        };


        glNamedFramebufferDrawBuffers(
            m_volumeFramebuffer,
            2,
            volumeDrawBuffers);


        GLenum status =
            glCheckNamedFramebufferStatus(
                m_volumeFramebuffer,
                GL_FRAMEBUFFER);


        if (status !=
            GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error(
                "Cloud2 raw volume framebuffer is incomplete.");
        }


        // =====================================================
        // FILTERED HALF-RES CLOUD
        // =====================================================

        glCreateFramebuffers(
            1,
            &m_spatialFramebuffer);


        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_spatialCloudTexture);


        glTextureStorage2D(
            m_spatialCloudTexture,
            1,
            GL_RGBA16F,
            m_volumeWidth,
            m_volumeHeight);


        glTextureParameteri(
            m_spatialCloudTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_spatialCloudTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_spatialCloudTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_spatialCloudTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_spatialDepthTexture);


        glTextureStorage2D(
            m_spatialDepthTexture,
            1,
            GL_R32F,
            m_volumeWidth,
            m_volumeHeight);


        glTextureParameteri(
            m_spatialDepthTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);


        glTextureParameteri(
            m_spatialDepthTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);


        glTextureParameteri(
            m_spatialDepthTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_spatialDepthTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glNamedFramebufferTexture(
            m_spatialFramebuffer,
            GL_COLOR_ATTACHMENT0,
            m_spatialCloudTexture,
            0);


        glNamedFramebufferTexture(
            m_spatialFramebuffer,
            GL_COLOR_ATTACHMENT1,
            m_spatialDepthTexture,
            0);


        const GLenum spatialDrawBuffers[2]
        {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1
        };


        glNamedFramebufferDrawBuffers(
            m_spatialFramebuffer,
            2,
            spatialDrawBuffers);


        status =
            glCheckNamedFramebufferStatus(
                m_spatialFramebuffer,
                GL_FRAMEBUFFER);


        if (status !=
            GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error(
                "Cloud2 spatial framebuffer is incomplete.");
        }


        // =====================================================
        // FULL-RES COMPOSITE
        // =====================================================

        glCreateFramebuffers(
            1,
            &m_compositeFramebuffer);


        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_compositeTexture);


        glTextureStorage2D(
            m_compositeTexture,
            1,
            GL_RGBA16F,
            width,
            height);


        glTextureParameteri(
            m_compositeTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_compositeTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_compositeTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_compositeTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glNamedFramebufferTexture(
            m_compositeFramebuffer,
            GL_COLOR_ATTACHMENT0,
            m_compositeTexture,
            0);


        glNamedFramebufferDrawBuffer(
            m_compositeFramebuffer,
            GL_COLOR_ATTACHMENT0);


        status =
            glCheckNamedFramebufferStatus(
                m_compositeFramebuffer,
                GL_FRAMEBUFFER);


        if (status !=
            GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error(
                "Cloud2 composite framebuffer is incomplete.");
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


        if (altitudeKm >
            5.0f)
        {
            return;
        }


        // =====================================================
        // FIX THE CLOUD TO THIS PLANET LOCATION
        // =====================================================

        m_formation.planetDirection =
            glm::normalize(
                cameraFromPlanetKm);


        // =====================================================
        // GENERATE THE LOCAL 3D DENSITY
        // =====================================================
        //
        // This happens ONCE.
        //
        // The formation generator can therefore be substantially
        // more expensive than something evaluated inside every
        // ray-march sample.

        m_densityVolume.generate(
            m_formation);


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
            << "[Cloud2] Local density volume: "
            << m_densityVolume.resolution()
            << "^3 R16F, "
            << m_densityVolume.mipCount()
            << " mips\n"
            << "[Cloud2] Cloud render resolution: "
            << m_volumeWidth
            << " x "
            << m_volumeHeight
            << "\n\n";
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


        resize(
            width,
            height);


        lockFormationIfNeeded(
            camera,
            *atmosphere);


        if (!m_formationLocked ||
            !m_formation.valid() ||
            !m_densityVolume.valid())
        {
            return
                sceneColorTexture;
        }


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
        // COMMON FULLSCREEN STATE
        // =====================================================

        glDisable(
            GL_DEPTH_TEST);


        glDepthMask(
            GL_FALSE);


        glDisable(
            GL_BLEND);


        glBindVertexArray(
            m_vertexArray);


        // =====================================================
        // PASS 1
        //
        // HALF-RES VOLUME RAYMARCH
        // =====================================================

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_volumeFramebuffer);


        glViewport(
            0,
            0,
            m_volumeWidth,
            m_volumeHeight);


        m_volumeShader.use();


        m_volumeShader.setMat4(
            "inverseViewProjection",
            rayReconstructionMatrix);


        m_volumeShader.setVec3(
            "cameraPositionWorld",
            camera.position);


        m_volumeShader.setVec3(
            "planetCenterWorld",
            atmosphere->planetCenterWorld);


        m_volumeShader.setFloat(
            "kmPerWorldUnit",
            kmPerWorldUnit);


        m_volumeShader.setFloat(
            "planetRadiusKm",
            parameters.bottomRadiusKm);


        m_volumeShader.setVec3(
            "formationDirection",
            glm::normalize(
                m_formation.planetDirection));


        m_volumeShader.setFloat(
            "formationBaseAltitudeKm",
            m_formation.baseAltitudeKm);


        m_volumeShader.setFloat(
            "formationHorizontalRadiusKm",
            m_formation.horizontalRadiusKm);


        m_volumeShader.setFloat(
            "formationHeightKm",
            m_formation.heightKm);


        m_volumeShader.setFloat(
            "formationDensityMultiplier",
            m_formation.densityMultiplier);


        m_volumeShader.setFloat(
            "cloudExtinctionPerKm",
            m_formation.extinctionPerKm);


        m_volumeShader.setFloat(
            "densityVolumeResolution",
            static_cast<float>(
                m_densityVolume.resolution()));


        m_volumeShader.setFloat(
            "densityVolumeMaxLod",
            m_densityVolume.maximumLod());


        m_volumeShader.setVec3(
            "sunDirection",
            glm::normalize(
                sun.direction));


        m_volumeShader.setVec3(
            "sunRadiance",
            sun.radiance);


        // Keep stochastic sampling frozen while we deliberately
        // postpone proper temporal reconstruction.

        m_volumeShader.setInt(
            "frameIndex",
            0);


        glBindTextureUnit(
            0,
            sceneLinearDepthTexture);


        glBindTextureUnit(
            1,
            m_densityVolume.texture());


        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);


        // =====================================================
        // PASS 2
        //
        // SAME-FRAME SPATIAL FILTER
        // =====================================================

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_spatialFramebuffer);


        glViewport(
            0,
            0,
            m_volumeWidth,
            m_volumeHeight);


        m_spatialShader.use();


        glBindTextureUnit(
            0,
            m_volumeCloudTexture);


        glBindTextureUnit(
            1,
            m_volumeDepthTexture);


        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);


        // =====================================================
        // PASS 3
        //
        // FULL-RES DEPTH-AWARE UPSCALE
        // =====================================================

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            m_compositeFramebuffer);


        glViewport(
            0,
            0,
            width,
            height);


        m_compositeShader.use();


        m_compositeShader.setFloat(
            "kmPerWorldUnit",
            kmPerWorldUnit);


        glBindTextureUnit(
            0,
            sceneColorTexture);


        glBindTextureUnit(
            1,
            m_spatialCloudTexture);


        glBindTextureUnit(
            2,
            sceneLinearDepthTexture);


        glBindTextureUnit(
            3,
            m_spatialDepthTexture);


        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);


        // =====================================================
        // RESTORE
        // =====================================================

        glBindVertexArray(
            0);


        glDepthMask(
            GL_TRUE);


        glEnable(
            GL_DEPTH_TEST);


        return
            m_compositeTexture;
    }
}