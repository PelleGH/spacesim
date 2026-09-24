#pragma once

#include "renderer/RenderCamera.h"

#include "renderer/atmosphere/AtmosphereInstance.h"

#include "renderer/cloud/CloudNoiseVolume.h"
#include "renderer/cloud/CloudWeatherMap.h"

#include "renderer/lighting/DirectionalLight.h"

#include "renderer/opengl/GlShader.h"

#include <glad/gl.h>

namespace SpaceSim
{
    class CloudPass
    {
    public:
        CloudPass();

        ~CloudPass();

        CloudPass(
            const CloudPass &) = delete;

        CloudPass &operator=(
            const CloudPass &) = delete;

        GLuint render(
            int width,
            int height,
            GLuint sceneColorTexture,
            GLuint sceneLinearDepthTexture,
            const RenderCamera &camera,
            float aspectRatio,
            const DirectionalLight &sun,
            const AtmosphereInstance &atmosphere);

        void setDebugView(int view) { m_debugView = view; }
        void setMorphologyDebugOverride(int mode) { m_morphologyDebugOverride = mode; }

    private:
        void resize(
            int width,
            int height);
        int m_debugView = 1;
        void destroyTarget();

        GlShader m_shader;

        // Local volumetric cloud structure.
        CloudNoiseVolume m_noiseVolume;

        // Planet-scale weather distribution.
        CloudWeatherMap m_weatherMap;
        int m_morphologyDebugOverride = 0;
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
    };
}