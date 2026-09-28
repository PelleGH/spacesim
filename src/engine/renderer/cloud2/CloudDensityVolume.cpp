#include "renderer/cloud2/CloudDensityVolume.h"

#include <stdexcept>


namespace SpaceSim
{
    CloudDensityVolume::CloudDensityVolume()
        : m_generator(
              "data/shaders/cloud2/cloud2_density_generate.comp")
    {
        glCreateTextures(
            GL_TEXTURE_3D,
            1,
            &m_texture);


        glTextureStorage3D(
            m_texture,
            MipCount,
            GL_R16F,
            Resolution,
            Resolution,
            Resolution);


        glTextureParameteri(
            m_texture,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR);


        glTextureParameteri(
            m_texture,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);


        glTextureParameteri(
            m_texture,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_texture,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_texture,
            GL_TEXTURE_WRAP_R,
            GL_CLAMP_TO_EDGE);


        glTextureParameteri(
            m_texture,
            GL_TEXTURE_BASE_LEVEL,
            0);


        glTextureParameteri(
            m_texture,
            GL_TEXTURE_MAX_LEVEL,
            MipCount -
                1);
    }


    CloudDensityVolume::~CloudDensityVolume()
    {
        if (m_texture != 0)
        {
            glDeleteTextures(
                1,
                &m_texture);


            m_texture =
                0;
        }
    }


    void CloudDensityVolume::generate(
        const CloudFormation& formation)
    {
        if (m_texture == 0)
        {
            throw std::runtime_error(
                "CloudDensityVolume has no texture.");
        }


        if (!formation.valid())
        {
            throw std::runtime_error(
                "Cannot generate an invalid cloud formation.");
        }


        m_generator.use();


        m_generator.setFloat(
            "formationHorizontalRadiusKm",
            formation.horizontalRadiusKm);


        m_generator.setFloat(
            "formationHeightKm",
            formation.heightKm);


        m_generator.setFloat(
            "formationSeed",
            formation.seed);


        // =====================================================
        // COMPUTE OUTPUT
        // =====================================================
        //
        // The compute shader writes mip level 0.
        //
        // GL_TRUE means this is a layered image, so the compute
        // shader can address every Z slice of the 3D texture.

        glBindImageTexture(
            0,
            m_texture,
            0,
            GL_TRUE,
            0,
            GL_WRITE_ONLY,
            GL_R16F);


        // cloud2_density_generate.comp uses:
        //
        //     local_size_x = 4
        //     local_size_y = 4
        //     local_size_z = 4
        //
        // 128 / 4 = 32 work groups on each axis.

        static constexpr GLuint GroupSize =
            4;


        const GLuint groupCount =
            (
                Resolution +
                GroupSize -
                1
            )
            /
            GroupSize;


        glDispatchCompute(
            groupCount,
            groupCount,
            groupCount);


        // Make compute writes visible before mip generation.

        glMemoryBarrier(
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_TEXTURE_FETCH_BARRIER_BIT);


        // =====================================================
        // GENERATE COARSER REPRESENTATIONS
        // =====================================================
        //
        // These mips become important immediately:
        //
        // view ray:
        //     chooses a mip based on sample footprint
        //
        // shadows:
        //     deliberately use coarse mips
        //
        // Therefore both paths are sampling the SAME cloud.

        glGenerateTextureMipmap(
            m_texture);


        glMemoryBarrier(
            GL_TEXTURE_FETCH_BARRIER_BIT);


        glBindImageTexture(
            0,
            0,
            0,
            GL_TRUE,
            0,
            GL_WRITE_ONLY,
            GL_R16F);


        m_generated =
            true;
    }
}