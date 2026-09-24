#pragma once

#include "renderer/RenderCamera.h"

#include "renderer/atmosphere/AtmosphereInstance.h"

#include "renderer/cloud2/CloudFormation.h"

#include "renderer/lighting/DirectionalLight.h"

#include "renderer/opengl/GlShader.h"

#include <glad/gl.h>


namespace SpaceSim
{
    class Cloud2TestPass
    {
    public:
        Cloud2TestPass();

        ~Cloud2TestPass();


        Cloud2TestPass(
            const Cloud2TestPass&) =
            delete;


        Cloud2TestPass& operator=(
            const Cloud2TestPass&) =
            delete;


        GLuint render(
            int width,
            int height,
            GLuint sceneColorTexture,
            GLuint sceneLinearDepthTexture,
            const RenderCamera& camera,
            float aspectRatio,
            const DirectionalLight& sun,
            const AtmosphereInstance* atmosphere);


    private:
        void resize(
            int width,
            int height);


        void destroyTarget();


        void lockFormationIfNeeded(
            const RenderCamera& camera,
            const AtmosphereInstance& atmosphere);


        GlShader m_shader;


        GLuint m_vertexArray =
            0;


        GLuint m_framebuffer =
            0;


        GLuint m_colorTexture =
            0;


        int m_width =
            0;


        int m_height =
            0;


        CloudFormation m_formation =
            makeTestCumulusFormation();


        bool m_formationLocked =
            false;
    };
}