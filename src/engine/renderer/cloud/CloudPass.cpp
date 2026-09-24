#include "renderer/cloud/CloudPass.h"

#include "renderer/CameraRayReconstruction.h"

#include "renderer/atmosphere/AtmosphereParameters.h"
#include "renderer/cloud/CloudParameters.h"

#include <glm/geometric.hpp>

#include <stdexcept>

namespace SpaceSim
{
    CloudPass::CloudPass()
        : m_shader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/cloud/cloud_debug.frag")
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


        m_shader.setInt(
            "baseShapeNoise",
            2);


        m_shader.setInt(
            "weatherMap",
            3);
    }


    CloudPass::~CloudPass()
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


    void CloudPass::destroyTarget()
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


    void CloudPass::resize(
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
                "Cloud framebuffer is incomplete.");
        }
    }


    GLuint CloudPass::render(
        int width,
        int height,
        GLuint sceneColorTexture,
        GLuint sceneLinearDepthTexture,
        const RenderCamera& camera,
        float aspectRatio,
        const DirectionalLight& sun,
        const AtmosphereInstance& atmosphere)
    {
        if (!atmosphere.cloudsValid())
        {
            return
                sceneColorTexture;
        }


        resize(
            width,
            height);


        const AtmosphereParameters& atmosphereParameters =
            *atmosphere.parameters;


        const CloudParameters& cloudParameters =
            *atmosphere.cloudParameters;


        const float kmPerWorldUnit =
            atmosphereParameters.bottomRadiusKm /
            atmosphere.planetRadiusWorld;


        const float cloudInnerRadiusKm =
            atmosphereParameters.bottomRadiusKm +
            cloudParameters.baseAltitudeKm;


        const float cloudOuterRadiusKm =
            atmosphereParameters.bottomRadiusKm +
            cloudParameters.topAltitudeKm;


        const glm::mat4 rayReconstructionMatrix =
            makeCameraRayReconstructionMatrix(
                camera,
                aspectRatio);


        // =========================================================
        // OUTPUT
        // =========================================================

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


        // =========================================================
        // SHADER PARAMETERS
        // =========================================================

        m_shader.use();


        m_shader.setMat4(
            "inverseViewProjection",
            rayReconstructionMatrix);


        m_shader.setVec3(
            "cameraPositionWorld",
            camera.position);


        m_shader.setVec3(
            "planetCenterWorld",
            atmosphere.planetCenterWorld);


        m_shader.setFloat(
            "kmPerWorldUnit",
            kmPerWorldUnit);


        m_shader.setFloat(
            "planetRadiusKm",
            atmosphereParameters.bottomRadiusKm);


        m_shader.setFloat(
            "cloudInnerRadiusKm",
            cloudInnerRadiusKm);


        m_shader.setFloat(
            "cloudOuterRadiusKm",
            cloudOuterRadiusKm);


        m_shader.setFloat(
            "cloudBoundaryFadeKm",
            cloudParameters.boundaryFadeKm);


        m_shader.setFloat(
            "cloudCoverage",
            cloudParameters.coverage);


        m_shader.setFloat(
            "coarseShapePeriodKm",
            cloudParameters.coarseShapePeriodKm);


        m_shader.setFloat(
            "baseShapePeriodKm",
            cloudParameters.baseShapePeriodKm);


        m_shader.setFloat(
            "cloudDensityMultiplier",
            cloudParameters.densityMultiplier);


        m_shader.setFloat(
            "localShapeFadeStartFootprintKm",
            cloudParameters.localShapeFadeStartFootprintKm);


        m_shader.setFloat(
            "localShapeFadeEndFootprintKm",
            cloudParameters.localShapeFadeEndFootprintKm);


        // =========================================================
        // SUN / CLOUD LIGHTING
        // =========================================================

        m_shader.setVec3(
            "sunDirection",
            glm::normalize(
                sun.direction));


        m_shader.setVec3(
            "sunRadiance",
            sun.radiance);


        m_shader.setFloat(
            "cloudExtinctionPerKm",
            cloudParameters.extinctionPerKm);


        m_shader.setVec3(
            "cloudScatteringAlbedo",
            cloudParameters.scatteringAlbedo);


        m_shader.setFloat(
            "shadowExtinctionMultiplier",
            cloudParameters.shadowExtinctionMultiplier);


        m_shader.setFloat(
            "forwardScatteringG",
            cloudParameters.forwardScatteringG);


        m_shader.setFloat(
            "backwardScatteringG",
            cloudParameters.backwardScatteringG);


        m_shader.setFloat(
            "forwardScatteringWeight",
            cloudParameters.forwardScatteringWeight);


        m_shader.setFloat(
            "cloudLightingIntensity",
            cloudParameters.lightingIntensity);


        // =========================================================
        // INPUT TEXTURES
        // =========================================================

        glBindTextureUnit(
            0,
            sceneColorTexture);


        glBindTextureUnit(
            1,
            sceneLinearDepthTexture);


        glBindTextureUnit(
            2,
            m_noiseVolume.baseShapeTexture());


        glBindTextureUnit(
            3,
            m_weatherMap.texture());


        // =========================================================
        // FULLSCREEN CLOUD PASS
        // =========================================================

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