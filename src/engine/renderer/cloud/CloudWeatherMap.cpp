#include "renderer/cloud/CloudWeatherMap.h"

namespace SpaceSim
{
    CloudWeatherMap::CloudWeatherMap()
        : m_generator(
              "data/shaders/cloud/cloud_weather.comp")
    {
        // Allow filtering cleanly across cubemap face boundaries.
        //
        // Without this, a cubemap can expose its six-face structure
        // as visible seams.
        glEnable(
            GL_TEXTURE_CUBE_MAP_SEAMLESS);


        generate();
    }


    void CloudWeatherMap::generate()
    {
        m_generator.use();


        // Fixed while developing so the same planet has the same
        // weather every launch.
        //
        // Eventually this becomes planet/world data.
        m_generator.setInt(
            "weatherSeed",
            7241);


        // A cubemap is internally six array layers.
        //
        // GL_TRUE means all six faces are available to the compute
        // shader rather than binding only one face.
        glBindImageTexture(
            0,
            m_texture.id(),
            0,
            GL_TRUE,
            0,
            GL_WRITE_ONLY,
            GL_RGBA16F);


        constexpr int localSize =
            8;


        const GLuint groupCount =
            static_cast<GLuint>(
                (
                    Resolution +
                    localSize -
                    1
                )
                /
                localSize);


        // Z = six cubemap faces.
        glDispatchCompute(
            groupCount,
            groupCount,
            6);


        // Make compute writes visible before OpenGL uses level zero
        // to construct the mip chain.
        glMemoryBarrier(
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_TEXTURE_UPDATE_BARRIER_BIT);


        glGenerateTextureMipmap(
            m_texture.id());


        glMemoryBarrier(
            GL_TEXTURE_FETCH_BARRIER_BIT);


        glBindImageTexture(
            0,
            0,
            0,
            GL_TRUE,
            0,
            GL_WRITE_ONLY,
            GL_RGBA16F);
    }
}