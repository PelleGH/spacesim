#include "renderer/cloud/CloudNoiseVolume.h"

namespace SpaceSim
{
    CloudNoiseVolume::CloudNoiseVolume()
        : m_baseShapeGenerator(
              "data/shaders/cloud/cloud_base_shape.comp")
    {
        generateBaseShape();
    }


    void CloudNoiseVolume::generateBaseShape()
    {
        m_baseShapeGenerator.use();


        m_baseShapeGenerator.setInt(
            "noiseSeed",
            1337);


        // The compute shader writes only mip level 0:
        //
        // 128 x 128 x 128

        glBindImageTexture(
            0,
            m_baseShapeTexture.id(),
            0,
            GL_TRUE,
            0,
            GL_WRITE_ONLY,
            GL_RGBA8);


        constexpr int localSize =
            4;


        const GLuint groupCount =
            static_cast<GLuint>(
                (
                    BaseShapeResolution +
                    localSize -
                    1
                )
                /
                localSize);


        glDispatchCompute(
            groupCount,
            groupCount,
            groupCount);


        // Make compute-shader writes visible before OpenGL reads
        // level 0 to construct the smaller mip levels.
        glMemoryBarrier(
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


        // Construct:
        //
        // 128³
        //  64³
        //  32³
        //  16³
        //   8³
        //   4³
        //   2³
        //   1³
        //
        // by progressively averaging the original volume.
        glGenerateTextureMipmap(
            m_baseShapeTexture.id());


        glMemoryBarrier(
            GL_TEXTURE_FETCH_BARRIER_BIT);


        glBindImageTexture(
            0,
            0,
            0,
            GL_TRUE,
            0,
            GL_WRITE_ONLY,
            GL_RGBA8);
    }
}