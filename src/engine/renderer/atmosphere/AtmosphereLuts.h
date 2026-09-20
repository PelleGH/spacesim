#pragma once

#include "renderer/atmosphere/AtmosphereParameters.h"

#include "renderer/opengl/GlComputeShader.h"
#include "renderer/opengl/GlTexture2D.h"

#include <glad/gl.h>


namespace SpaceSim
{
    // =============================================================
    // CAMERA-INDEPENDENT ATMOSPHERE LUTS
    // =============================================================
    //
    // These depend only on the physical atmosphere model.
    //
    // They do NOT need to be regenerated when:
    //
    // - the camera moves
    // - the ship moves
    // - the sun changes direction
    //
    // because direction-dependent quantities such as sunMu are
    // dimensions inside the LUTs themselves.

    class AtmosphereLuts
    {
    public:
        explicit AtmosphereLuts(
            const AtmosphereParameters& atmosphere)
            : m_transmittance(
                  256,
                  64,
                  GL_RGBA16F),

              m_multipleScattering(
                  32,
                  32,
                  GL_RGBA16F),

              m_skyIrradiance(
                  64,
                  32,
                  GL_RGBA16F),

              m_transmittanceShader(
                  "data/shaders/atmosphere/transmittance.comp"),

              m_multipleScatteringShader(
                  "data/shaders/atmosphere/multiple_scattering.comp"),

              m_skyIrradianceShader(
                  "data/shaders/atmosphere/sky_irradiance.comp")
        {
            regenerate(
                atmosphere);
        }


        AtmosphereLuts(
            const AtmosphereLuts&) = delete;


        AtmosphereLuts& operator=(
            const AtmosphereLuts&) = delete;


        void regenerate(
            const AtmosphereParameters& atmosphere)
        {
            // Order matters:
            //
            // transmittance
            //      ↓
            // multiple scattering
            //      ↓
            // sky irradiance

            generateTransmittance(
                atmosphere);

            generateMultipleScattering(
                atmosphere);

            generateSkyIrradiance(
                atmosphere);
        }


        const GlTexture2D& transmittance() const
        {
            return
                m_transmittance;
        }


        const GlTexture2D& multipleScattering() const
        {
            return
                m_multipleScattering;
        }


        const GlTexture2D& skyIrradiance() const
        {
            return
                m_skyIrradiance;
        }


    private:
        // =========================================================
        // SHARED ATMOSPHERE UNIFORMS
        // =========================================================

        static void setAtmosphereUniforms(
            const GlComputeShader& shader,
            const AtmosphereParameters& atmosphere)
        {
            shader.setFloat(
                "bottomRadiusKm",
                atmosphere.bottomRadiusKm);


            shader.setFloat(
                "topRadiusKm",
                atmosphere.topRadiusKm);


            shader.setVec3(
                "rayleighScatteringPerKm",
                atmosphere.rayleighScatteringPerKm);


            shader.setFloat(
                "rayleighScaleHeightKm",
                atmosphere.rayleighScaleHeightKm);


            shader.setVec3(
                "mieScatteringPerKm",
                atmosphere.mieScatteringPerKm);


            shader.setVec3(
                "mieExtinctionPerKm",
                atmosphere.mieExtinctionPerKm);


            shader.setFloat(
                "mieScaleHeightKm",
                atmosphere.mieScaleHeightKm);


            shader.setFloat(
                "mieAnisotropy",
                atmosphere.mieAnisotropy);


            shader.setVec3(
                "ozoneAbsorptionPerKm",
                atmosphere.ozoneAbsorptionPerKm);


            shader.setFloat(
                "ozoneCenterHeightKm",
                atmosphere.ozoneCenterHeightKm);


            shader.setFloat(
                "ozoneHalfWidthKm",
                atmosphere.ozoneHalfWidthKm);


            shader.setVec3(
                "groundAlbedo",
                atmosphere.groundAlbedo);
        }


        // =========================================================
        // TRANSMITTANCE
        // =========================================================

        void generateTransmittance(
            const AtmosphereParameters& atmosphere)
        {
            m_transmittanceShader.use();


            setAtmosphereUniforms(
                m_transmittanceShader,
                atmosphere);


            glBindImageTexture(
                0,
                m_transmittance.id(),
                0,
                GL_FALSE,
                0,
                GL_WRITE_ONLY,
                GL_RGBA16F);


            const GLuint groupsX =
                (
                    static_cast<GLuint>(
                        m_transmittance.width())
                    +
                    7u
                )
                /
                8u;


            const GLuint groupsY =
                (
                    static_cast<GLuint>(
                        m_transmittance.height())
                    +
                    7u
                )
                /
                8u;


            glDispatchCompute(
                groupsX,
                groupsY,
                1);


            glMemoryBarrier(
                GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                GL_TEXTURE_FETCH_BARRIER_BIT);
        }


        // =========================================================
        // MULTIPLE SCATTERING
        // =========================================================

        void generateMultipleScattering(
            const AtmosphereParameters& atmosphere)
        {
            m_multipleScatteringShader.use();


            setAtmosphereUniforms(
                m_multipleScatteringShader,
                atmosphere);


            m_multipleScatteringShader.setInt(
                "transmittanceLut",
                2);


            glBindTextureUnit(
                2,
                m_transmittance.id());


            glBindImageTexture(
                0,
                m_multipleScattering.id(),
                0,
                GL_FALSE,
                0,
                GL_WRITE_ONLY,
                GL_RGBA16F);


            const GLuint groupsX =
                (
                    static_cast<GLuint>(
                        m_multipleScattering.width())
                    +
                    7u
                )
                /
                8u;


            const GLuint groupsY =
                (
                    static_cast<GLuint>(
                        m_multipleScattering.height())
                    +
                    7u
                )
                /
                8u;


            glDispatchCompute(
                groupsX,
                groupsY,
                1);


            glMemoryBarrier(
                GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                GL_TEXTURE_FETCH_BARRIER_BIT);
        }


        // =========================================================
        // SKY IRRADIANCE
        // =========================================================
        //
        // This integrates atmospheric sky radiance over the local
        // upper hemisphere.
        //
        // The result is diffuse irradiance for unit stellar
        // radiance. The actual sunRadiance is multiplied later by
        // the PBR shader.

        void generateSkyIrradiance(
            const AtmosphereParameters& atmosphere)
        {
            m_skyIrradianceShader.use();


            setAtmosphereUniforms(
                m_skyIrradianceShader,
                atmosphere);


            m_skyIrradianceShader.setInt(
                "transmittanceLut",
                2);


            m_skyIrradianceShader.setInt(
                "multipleScatteringLut",
                3);


            glBindTextureUnit(
                2,
                m_transmittance.id());


            glBindTextureUnit(
                3,
                m_multipleScattering.id());


            glBindImageTexture(
                0,
                m_skyIrradiance.id(),
                0,
                GL_FALSE,
                0,
                GL_WRITE_ONLY,
                GL_RGBA16F);


            const GLuint groupsX =
                (
                    static_cast<GLuint>(
                        m_skyIrradiance.width())
                    +
                    7u
                )
                /
                8u;


            const GLuint groupsY =
                (
                    static_cast<GLuint>(
                        m_skyIrradiance.height())
                    +
                    7u
                )
                /
                8u;


            glDispatchCompute(
                groupsX,
                groupsY,
                1);


            glMemoryBarrier(
                GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                GL_TEXTURE_FETCH_BARRIER_BIT);
        }


        // =========================================================
        // TEXTURES
        // =========================================================

        GlTexture2D m_transmittance;

        GlTexture2D m_multipleScattering;

        GlTexture2D m_skyIrradiance;


        // =========================================================
        // GENERATORS
        // =========================================================

        GlComputeShader m_transmittanceShader;

        GlComputeShader m_multipleScatteringShader;

        GlComputeShader m_skyIrradianceShader;
    };
}