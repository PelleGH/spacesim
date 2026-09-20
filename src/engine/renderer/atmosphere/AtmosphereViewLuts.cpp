#include "renderer/atmosphere/AtmosphereViewLuts.h"

#include "renderer/atmosphere/AtmosphereLuts.h"
#include "renderer/atmosphere/AtmosphereParameters.h"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>


namespace SpaceSim
{
    AtmosphereViewLuts::AtmosphereViewLuts()
        : m_skyViewShader(
              "data/shaders/atmosphere/sky_view_runtime.comp"),

          m_skyReflectionPrefilterShader(
              "data/shaders/atmosphere/sky_reflection_prefilter.comp")
    {
        createTextures();


        // =========================================================
        // SKY VIEW
        // =========================================================

        m_skyViewShader.setInt(
            "transmittanceLut",
            2);

        m_skyViewShader.setInt(
            "multipleScatteringLut",
            3);


        // =========================================================
        // SKY REFLECTION PREFILTER
        // =========================================================

        m_skyReflectionPrefilterShader.setInt(
            "skyViewLut",
            2);

        m_skyReflectionPrefilterShader.setInt(
            "transmittanceLut",
            3);

        m_skyReflectionPrefilterShader.setInt(
            "skyIrradianceLut",
            4);
    }


    AtmosphereViewLuts::~AtmosphereViewLuts()
    {
        destroyTextures();
    }


    void AtmosphereViewLuts::createTextures()
    {
        // =========================================================
        // RAW SKY-VIEW LUT
        // =========================================================
        //
        // This stores the actual directional atmosphere visible
        // from the current camera position.
        //
        // AtmospherePass samples this directly for background sky.
        //
        // Reflective surfaces use the separately filtered texture
        // below.

        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_skyViewTexture);


        glTextureStorage2D(
            m_skyViewTexture,
            1,
            GL_RGBA16F,
            SkyWidth,
            SkyHeight);


        glTextureParameteri(
            m_skyViewTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);


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
        // GGX-PREFILTERED SKY REFLECTION
        // =========================================================
        //
        // Mip 0:
        //
        //     sharp atmosphere + ground
        //
        // Higher mips:
        //
        //     progressively rougher GGX-filtered reflections.
        //
        // This remains useful for:
        //
        // - oceans
        // - ships
        // - metallic surfaces
        // - future wet terrain

        glCreateTextures(
            GL_TEXTURE_2D,
            1,
            &m_skyReflectionTexture);


        glTextureStorage2D(
            m_skyReflectionTexture,
            SkyMipLevels,
            GL_RGBA16F,
            SkyWidth,
            SkyHeight);


        glTextureParameteri(
            m_skyReflectionTexture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR);


        glTextureParameteri(
            m_skyReflectionTexture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_skyReflectionTexture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_skyReflectionTexture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);
    }


    void AtmosphereViewLuts::destroyTextures()
    {
        if (m_skyReflectionTexture != 0)
        {
            glDeleteTextures(
                1,
                &m_skyReflectionTexture);


            m_skyReflectionTexture =
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
            parameters.bottomRadiusKm /
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


        shader.setVec3(
            "groundAlbedo",
            parameters.groundAlbedo);
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
        // 1. RAW SKY-VIEW LUT
        // =========================================================
        //
        // This is still view-dependent, so it is regenerated for
        // the current camera.
        //
        // Unlike the removed aerial volume, this LUT contains
        // direction only -- there is no discrete scene-distance
        // dimension that can create camera-centered distance rings.

        m_skyViewShader.use();


        setCommonUniforms(
            m_skyViewShader,
            camera,
            aspectRatio,
            sun,
            atmosphere);


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


        // =========================================================
        // 2. GGX-PREFILTERED REFLECTION ENVIRONMENT
        // =========================================================
        //
        // This produces the roughness mip-chain used by reflective
        // surfaces.
        //
        // There is deliberately NO aerial-perspective 3D volume
        // generation after this anymore.

        m_skyReflectionPrefilterShader.use();


        setCommonUniforms(
            m_skyReflectionPrefilterShader,
            camera,
            aspectRatio,
            sun,
            atmosphere);


        glBindTextureUnit(
            2,
            m_skyViewTexture);


        glBindTextureUnit(
            3,
            staticLuts
                .transmittance()
                .id());


        glBindTextureUnit(
            4,
            staticLuts
                .skyIrradiance()
                .id());


        for (int mipLevel = 0;
             mipLevel < SkyMipLevels;
             ++mipLevel)
        {
            const int mipWidth =
                std::max(
                    1,
                    SkyWidth >> mipLevel);


            const int mipHeight =
                std::max(
                    1,
                    SkyHeight >> mipLevel);


            const float mipRoughness =
                SkyMipLevels > 1
                    ?
                    static_cast<float>(
                        mipLevel)
                    /
                    static_cast<float>(
                        SkyMipLevels - 1)
                    :
                    0.0f;


            m_skyReflectionPrefilterShader.setFloat(
                "roughness",
                mipRoughness);


            // Mip 0 is the exact sharp environment.
            //
            // Higher mips perform the GGX convolution.
            m_skyReflectionPrefilterShader.setInt(
                "sampleCount",
                mipLevel == 0
                    ?
                    1
                    :
                    64);


            glBindImageTexture(
                0,
                m_skyReflectionTexture,
                mipLevel,
                GL_FALSE,
                0,
                GL_WRITE_ONLY,
                GL_RGBA16F);


            glDispatchCompute(
                (mipWidth + 7) / 8,
                (mipHeight + 7) / 8,
                1);
        }


        glMemoryBarrier(
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_TEXTURE_FETCH_BARRIER_BIT);
    }
}