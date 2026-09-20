#pragma once

#include "renderer/opengl/GlComputeShader.h"
#include "renderer/opengl/GlTexture2D.h"
#include "renderer/opengl/GlTextureCube.h"

#include <glad/gl.h>

#include <algorithm>

namespace SpaceSim
{
    class EnvironmentIbl
    {
    public:
        explicit EnvironmentIbl(
            const GlTextureCube& environmentMap)
            : m_irradianceMap(
                  32,
                  1),
              m_prefilteredMap(
                  64,
                  7),
              m_brdfLut(
                  256,
                  256,
                  GL_RG16F),
              m_irradianceShader(
                  "data/shaders/renderer/irradiance.comp"),
              m_prefilterShader(
                  "data/shaders/renderer/prefilter_environment.comp"),
              m_brdfShader(
                  "data/shaders/renderer/brdf_lut.comp")
        {
            regenerate(
                environmentMap);
        }


        EnvironmentIbl(
            const EnvironmentIbl&) = delete;

        EnvironmentIbl& operator=(
            const EnvironmentIbl&) = delete;


        void regenerate(
            const GlTextureCube& environmentMap)
        {
            generateIrradiance(
                environmentMap);

            generatePrefilteredEnvironment(
                environmentMap);

            generateBrdfLut();
        }


        const GlTextureCube& irradianceMap() const
        {
            return m_irradianceMap;
        }


        const GlTextureCube& prefilteredMap() const
        {
            return m_prefilteredMap;
        }


        const GlTexture2D& brdfLut() const
        {
            return m_brdfLut;
        }


    private:
        void generateIrradiance(
            const GlTextureCube& environmentMap)
        {
            m_irradianceShader.use();

            glBindTextureUnit(
                2,
                environmentMap.id());

            glBindImageTexture(
                0,
                m_irradianceMap.id(),
                0,
                GL_TRUE,
                0,
                GL_WRITE_ONLY,
                GL_RGBA16F);


            const GLuint resolution =
                static_cast<GLuint>(
                    m_irradianceMap.resolution());

            const GLuint groupCount =
                (
                    resolution +
                    7u
                ) /
                8u;


            glDispatchCompute(
                groupCount,
                groupCount,
                6);


            glMemoryBarrier(
                GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                GL_TEXTURE_FETCH_BARRIER_BIT);
        }


        void generatePrefilteredEnvironment(
            const GlTextureCube& environmentMap)
        {
            m_prefilterShader.use();

            m_prefilterShader.setInt(
                "environmentMap",
                2);

            glBindTextureUnit(
                2,
                environmentMap.id());


            const int mipCount =
                m_prefilteredMap.mipLevels();


            for (int mip = 0;
                 mip < mipCount;
                 ++mip)
            {
                const float roughness =
                    mipCount > 1
                        ? static_cast<float>(mip) /
                          static_cast<float>(
                              mipCount - 1)
                        : 0.0f;


                m_prefilterShader.setFloat(
                    "roughness",
                    roughness);


                glBindImageTexture(
                    0,
                    m_prefilteredMap.id(),
                    mip,
                    GL_TRUE,
                    0,
                    GL_WRITE_ONLY,
                    GL_RGBA16F);


                const int mipResolution =
                    std::max(
                        1,
                        m_prefilteredMap.resolution() >>
                        mip);


                const GLuint groupCount =
                    (
                        static_cast<GLuint>(
                            mipResolution) +
                        7u
                    ) /
                    8u;


                glDispatchCompute(
                    groupCount,
                    groupCount,
                    6);
            }


            glMemoryBarrier(
                GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                GL_TEXTURE_FETCH_BARRIER_BIT);
        }


        void generateBrdfLut()
        {
            m_brdfShader.use();


            glBindImageTexture(
                0,
                m_brdfLut.id(),
                0,
                GL_FALSE,
                0,
                GL_WRITE_ONLY,
                GL_RG16F);


            const GLuint groupsX =
                (
                    static_cast<GLuint>(
                        m_brdfLut.width()) +
                    7u
                ) /
                8u;


            const GLuint groupsY =
                (
                    static_cast<GLuint>(
                        m_brdfLut.height()) +
                    7u
                ) /
                8u;


            glDispatchCompute(
                groupsX,
                groupsY,
                1);


            glMemoryBarrier(
                GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                GL_TEXTURE_FETCH_BARRIER_BIT);
        }


        GlTextureCube m_irradianceMap;

        GlTextureCube m_prefilteredMap;

        GlTexture2D m_brdfLut;


        GlComputeShader m_irradianceShader;

        GlComputeShader m_prefilterShader;

        GlComputeShader m_brdfShader;
    };
}