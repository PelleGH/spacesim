#pragma once

#include "renderer/opengl/GlComputeShader.h"
#include "renderer/opengl/GlTexture3D.h"

#include <glad/gl.h>

namespace SpaceSim
{
    class CloudNoiseVolume
    {
    public:
        CloudNoiseVolume();


        GLuint baseShapeTexture() const
        {
            return
                m_baseShapeTexture.id();
        }


        static constexpr int BaseShapeResolution =
            128;


    private:
        void generateBaseShape();


        // RGBA8 instead of just R8 is intentional.
        //
        // R = final combined cloud shape
        // G = gradient / Perlin-style FBM
        // B = Worley FBM
        // A = unused / 1
        //
        // We only consume R right now, but keeping the intermediate
        // channels makes shader debugging much easier.

        GlTexture3D m_baseShapeTexture
        {
            BaseShapeResolution,
            BaseShapeResolution,
            BaseShapeResolution,
            GL_RGBA8
        };


        GlComputeShader m_baseShapeGenerator;
    };
}