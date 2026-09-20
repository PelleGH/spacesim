#include "renderer/atmosphere/AtmospherePass.h"

#include "renderer/CameraRayReconstruction.h"

#include "renderer/atmosphere/AtmosphereLuts.h"
#include "renderer/atmosphere/AtmosphereParameters.h"

#include <glm/geometric.hpp>

#include <stdexcept>


namespace SpaceSim
{
    AtmospherePass::AtmospherePass()
        : m_shader(
              "data/shaders/renderer/fullscreen.vert",
              "data/shaders/atmosphere/atmosphere_planet.frag")
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
            "transmittanceLut",
            2);


        m_shader.setInt(
            "multipleScatteringLut",
            3);


        m_shader.setInt(
            "skyViewTexture",
            4);
    }


    AtmospherePass::~AtmospherePass()
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


    void AtmospherePass::destroyTarget()
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


    void AtmospherePass::resize(
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
                "Atmosphere framebuffer is incomplete.");
        }
    }


    GLuint AtmospherePass::render(
        int width,
        int height,
        GLuint sceneColorTexture,
        GLuint sceneLinearDepthTexture,
        const RenderCamera& camera,
        float aspectRatio,
        const DirectionalLight& sun,
        const AtmosphereInstance& atmosphere,
        const AtmosphereViewLuts& viewLuts)
    {
        if (!atmosphere.valid())
        {
            return
                sceneColorTexture;
        }


        resize(
            width,
            height);


        const AtmosphereParameters& parameters =
            *atmosphere.parameters;


        const AtmosphereLuts& staticLuts =
            *atmosphere.luts;


        const glm::mat4 rayReconstructionMatrix =
            makeCameraRayReconstructionMatrix(
                camera,
                aspectRatio);


        const float kmPerWorldUnit =
            parameters.bottomRadiusKm
            /
            atmosphere.planetRadiusWorld;


        // =========================================================
        // OUTPUT TARGET
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


        m_shader.setVec3(
            "sunDirection",
            glm::normalize(
                sun.direction));


        m_shader.setVec3(
            "sunRadiance",
            sun.radiance);


        m_shader.setFloat(
            "kmPerWorldUnit",
            kmPerWorldUnit);


        m_shader.setFloat(
            "bottomRadiusKm",
            parameters.bottomRadiusKm);


        m_shader.setFloat(
            "topRadiusKm",
            parameters.topRadiusKm);


        m_shader.setVec3(
            "rayleighScatteringPerKm",
            parameters.rayleighScatteringPerKm);


        m_shader.setFloat(
            "rayleighScaleHeightKm",
            parameters.rayleighScaleHeightKm);


        m_shader.setVec3(
            "mieScatteringPerKm",
            parameters.mieScatteringPerKm);


        m_shader.setVec3(
            "mieExtinctionPerKm",
            parameters.mieExtinctionPerKm);


        m_shader.setFloat(
            "mieScaleHeightKm",
            parameters.mieScaleHeightKm);


        m_shader.setFloat(
            "mieAnisotropy",
            parameters.mieAnisotropy);


        m_shader.setVec3(
            "ozoneAbsorptionPerKm",
            parameters.ozoneAbsorptionPerKm);


        m_shader.setFloat(
            "ozoneCenterHeightKm",
            parameters.ozoneCenterHeightKm);


        m_shader.setFloat(
            "ozoneHalfWidthKm",
            parameters.ozoneHalfWidthKm);


        // =========================================================
        // INPUT TEXTURES
        // =========================================================
        //
        // The old final pass sampled:
        //
        //     aerialScatteringTexture
        //     aerialTransmittanceTexture
        //
        // Those are deliberately NOT bound anymore.
        //
        // The exact camera-to-fragment atmosphere segment is now
        // integrated directly by atmosphere_planet.frag.

        glBindTextureUnit(
            0,
            sceneColorTexture);


        glBindTextureUnit(
            1,
            sceneLinearDepthTexture);


        glBindTextureUnit(
            2,
            staticLuts
                .transmittance()
                .id());


        glBindTextureUnit(
            3,
            staticLuts
                .multipleScattering()
                .id());


        glBindTextureUnit(
            4,
            viewLuts.skyViewTexture());


        // =========================================================
        // FULLSCREEN COMPOSITE
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