#pragma once

#include "renderer/opengl/GlComputeShader.h"
#include "renderer/opengl/GlTextureCube.h"

#include <glad/gl.h>

namespace SpaceSim
{
    class CloudWeatherMap
    {
    public:
        static constexpr int Resolution =
            256;


        // 256 -> 128 -> 64 -> 32 -> 16 -> 8 -> 4 -> 2 -> 1
        static constexpr int MipLevels =
            9;


        CloudWeatherMap();


        GLuint texture() const
        {
            return
                m_texture.id();
        }


    private:
        void generate();


        // RGBA16F is already what GlTextureCube uses.
        //
        // Channels:
        //
        // R = cloud coverage
        // G = cloud type
        // B = storminess
        // A = height potential
        //
        // Only some of these are used immediately, but this gives us
        // a useful weather-data layout going forward.

        GlTextureCube m_texture
        {
            Resolution,
            MipLevels
        };


        GlComputeShader m_generator;
    };
}