#pragma once

#include "renderer/cloud2/CloudFormation.h"
#include "renderer/opengl/GlComputeShader.h"

#include <glad/gl.h>


namespace SpaceSim
{
    class CloudDensityVolume
    {
    public:
        static constexpr int Resolution =
            128;


        static constexpr int MipCount =
            8;


        CloudDensityVolume();

        ~CloudDensityVolume();


        CloudDensityVolume(
            const CloudDensityVolume&) =
            delete;


        CloudDensityVolume& operator=(
            const CloudDensityVolume&) =
            delete;


        void generate(
            const CloudFormation& formation);


        GLuint texture() const
        {
            return
                m_texture;
        }


        int resolution() const
        {
            return
                Resolution;
        }


        int mipCount() const
        {
            return
                MipCount;
        }


        float maximumLod() const
        {
            return
                static_cast<float>(
                    MipCount -
                    1);
        }


        bool valid() const
        {
            return
                m_texture != 0 &&
                m_generated;
        }


    private:
        GlComputeShader m_generator;


        GLuint m_texture =
            0;


        bool m_generated =
            false;
    };
}