#include "renderer/atmosphere/AtmosphereViewLuts.h"

#include "renderer/atmosphere/AtmosphereLuts.h"
#include "renderer/atmosphere/AtmosphereParameters.h"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>


namespace SpaceSim
{
    AtmosphereViewLuts::AtmosphereViewLuts()
        : m_skyViewShader(
              "data/shaders/atmosphere/sky_view_runtime.comp"),

          m_aerialPerspectiveShader(
              "data/shaders/atmosphere/aerial_perspective.comp")
    {
        createTextures();


        m_skyViewShader.setInt(
            "transmittanceLut",
            2);


        m_skyViewShader.setInt(
            "multipleScatteringLut",
            3);


        m_aerialPerspectiveShader.setInt(
            "transmittanceLut",
            2);


        m_aerialPerspectiveShader.setInt(
            "multipleScatteringLut",
            3);
    }


    AtmosphereViewLuts::~AtmosphereViewLuts()
    {
        destroyTextures();
    }


    void AtmosphereViewLuts::createTextures()
    {
        // =========================================================
        // SKY-VIEW LUT
        // =========================================================
        //
        // The Sky-View LUT is an angular atmosphere representation.
        //
        // We also create a mip chain because the PBR pass will use
        // higher mip levels as a first approximation of rough
        // atmospheric reflections.
        //
        // 320 -> 160 -> 80 -> 40 -> 20 -> 10 -> 5 -> 2 -> 1
        //
        // therefore nine levels are sufficient.

        constexpr GLint skyMipLevels =
            9;


        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_skyViewTexture);


        glTextureStorage2D(
            m_skyViewTexture,
            skyMipLevels,
            GL_RGBA16F,
            SkyWidth,
            SkyHeight);


        glTextureParameteri(
            m_skyViewTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR);


        glTextureParameteri(
            m_skyViewTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_skyViewTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_skyViewTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        // =========================================================
        // AERIAL-PERSPECTIVE VOLUME
        // =========================================================

        glCreateTextures(
            GL_TEXTURE_3D,
            1,
            &m_aerialScatteringTexture);


        glTextureStorage3D(
            m_aerialScatteringTexture,
            1,
            GL_RGBA16F,
            AerialWidth,
            AerialHeight,
            AerialDepth);


        glTextureParameteri(
            m_aerialScatteringTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_aerialScatteringTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_aerialScatteringTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_aerialScatteringTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_aerialScatteringTexture,
            GL_TEXTURE_WRAP_R,
            GL_CLAMP_TO_EDGE);


        glCreateTextures(
            GL_TEXTURE_3D,
            1,
            &m_aerialTransmittanceTexture);


        glTextureStorage3D(
            m_aerialTransmittanceTexture,
            1,
            GL_RGBA16F,
            AerialWidth,
            AerialHeight,
            AerialDepth);


        glTextureParameteri(
            m_aerialTransmittanceTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_aerialTransmittanceTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_aerialTransmittanceTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_aerialTransmittanceTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_aerialTransmittanceTexture,
            GL_TEXTURE_WRAP_R,
            GL_CLAMP_TO_EDGE);
    }


    void AtmosphereViewLuts::destroyTextures()
    {
        if (m_aerialTransmittanceTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_aerialTransmittanceTexture);


            m_aerialTransmittanceTexture =
                0;
        }


        if (m_aerialScatteringTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_aerialScatteringTexture);


            m_aerialScatteringTexture =
                0;
        }


        if (m_skyViewTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_skyViewTexture);


            m_skyViewTexture =
                0;
        }
    }


    void AtmosphereViewLuts::setCommonUniforms(
        const GlComputeShader& shader,
        const RenderCamera& camera,
        float aspectRatio,
        const DirectionalLight& sun,
        const AtmosphereInstance& atmosphere) const
    {
        const AtmosphereParameters& parameters =
            *atmosphere.parameters;


        const glm::vec3 forward =
            glm::normalize(
                camera.forward);


        const glm::vec3 right =
            glm::normalize(
                glm::cross(
                    forward,
                    camera.up));


        const glm::vec3 correctedUp =
            glm::normalize(
                glm::cross(
                    right,
                    forward));


        const float tanHalfFov =
            std::tan(
                glm::radians(
                    camera.verticalFovDegrees)
                *
                0.5f);


        const float kmPerWorldUnit =
            parameters.bottomRadiusKm
            /
            atmosphere.planetRadiusWorld;


        shader.setVec3(
            "cameraPositionWorld",
            camera.position);


        shader.setVec3(
            "cameraForward",
            forward);


        shader.setVec3(
            "cameraRight",
            right);


        shader.setVec3(
            "cameraUp",
            correctedUp);


        shader.setFloat(
            "tanHalfFov",
            tanHalfFov);


        shader.setFloat(
            "aspectRatio",
            aspectRatio);


        shader.setVec3(
            "planetCenterWorld",
            atmosphere.planetCenterWorld);


        shader.setFloat(
            "kmPerWorldUnit",
            kmPerWorldUnit);


        shader.setFloat(
            "bottomRadiusKm",
            parameters.bottomRadiusKm);


        shader.setFloat(
            "topRadiusKm",
            parameters.topRadiusKm);


        shader.setVec3(
            "rayleighScatteringPerKm",
            parameters.rayleighScatteringPerKm);


        shader.setFloat(
            "rayleighScaleHeightKm",
            parameters.rayleighScaleHeightKm);


        shader.setVec3(
            "mieScatteringPerKm",
            parameters.mieScatteringPerKm);


        shader.setVec3(
            "mieExtinctionPerKm",
            parameters.mieExtinctionPerKm);


        shader.setFloat(
            "mieScaleHeightKm",
            parameters.mieScaleHeightKm);


        shader.setFloat(
            "mieAnisotropy",
            parameters.mieAnisotropy);


        shader.setVec3(
            "ozoneAbsorptionPerKm",
            parameters.ozoneAbsorptionPerKm);


        shader.setFloat(
            "ozoneCenterHeightKm",
            parameters.ozoneCenterHeightKm);


        shader.setFloat(
            "ozoneHalfWidthKm",
            parameters.ozoneHalfWidthKm);


        shader.setVec3(
            "sunDirection",
            glm::normalize(
                sun.direction));


        shader.setVec3(
            "sunRadiance",
            sun.radiance);
    }


    void AtmosphereViewLuts::update(
        const RenderCamera& camera,
        float aspectRatio,
        const DirectionalLight& sun,
        const AtmosphereInstance& atmosphere)
    {
        if (!atmosphere.valid())
        {
            return;
        }


        const AtmosphereLuts& staticLuts =
            *atmosphere.luts;


        // =========================================================
        // SKY-VIEW LUT
        // =========================================================

        m_skyViewShader.use();


        setCommonUniforms(
            m_skyViewShader,
            camera,
            aspectRatio,
            sun,
            atmosphere);


        glBindTextureUnit(
            2,
            staticLuts.transmittance().id());


        glBindTextureUnit(
            3,
            staticLuts.multipleScattering().id());


        // Compute shader writes only mip 0.
        glBindImageTexture(
            0,
            m_skyViewTexture,
            0,
            GL_FALSE,
            0,
            GL_WRITE_ONLY,
            GL_RGBA16F);


        glDispatchCompute(
            (SkyWidth + 7) / 8,
            (SkyHeight + 7) / 8,
            1);


        glMemoryBarrier(
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_TEXTURE_FETCH_BARRIER_BIT);


        // Generate blurred versions for rough reflections.
        //
        // Later this should become a proper GGX convolution rather
        // than a normal mipmap chain.

        glGenerateTextureMipmap(
            m_skyViewTexture);


        glMemoryBarrier(
            GL_TEXTURE_FETCH_BARRIER_BIT);


        // =========================================================
        // AERIAL-PERSPECTIVE VOLUME
        // =========================================================

        m_aerialPerspectiveShader.use();


        setCommonUniforms(
            m_aerialPerspectiveShader,
            camera,
            aspectRatio,
            sun,
            atmosphere);


        glBindTextureUnit(
            2,
            staticLuts.transmittance().id());


        glBindTextureUnit(
            3,
            staticLuts.multipleScattering().id());


        glBindImageTexture(
            0,
            m_aerialScatteringTexture,
            0,
            GL_TRUE,
            0,
            GL_WRITE_ONLY,
            GL_RGBA16F);


        glBindImageTexture(
            1,
            m_aerialTransmittanceTexture,
            0,
            GL_TRUE,
            0,
            GL_WRITE_ONLY,
            GL_RGBA16F);


        glDispatchCompute(
            (AerialWidth + 3) / 4,
            (AerialHeight + 3) / 4,
            (AerialDepth + 3) / 4);


        glMemoryBarrier(
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_TEXTURE_FETCH_BARRIER_BIT);
    }
}