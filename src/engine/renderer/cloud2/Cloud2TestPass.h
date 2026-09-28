#pragma once

#include "renderer/RenderCamera.h"

#include "renderer/atmosphere/AtmosphereInstance.h"
#include "renderer/cloud2/CloudDensityVolume.h"
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


        void destroyTargets();


        void lockFormationIfNeeded(
            const RenderCamera& camera,
            const AtmosphereInstance& atmosphere);


        // =====================================================
        // LOCAL CLOUD DENSITY
        // =====================================================

        CloudDensityVolume m_densityVolume;


        // =====================================================
        // PASS 1
        //
        // Half-resolution volume integration.
        // =====================================================

        GlShader m_volumeShader;


        // =====================================================
        // PASS 2
        //
        // Current same-frame spatial filter.
        // =====================================================

        GlShader m_spatialShader;


        // =====================================================
        // PASS 3
        //
        // Full-resolution depth-aware composite.
        // =====================================================

        GlShader m_compositeShader;


        GLuint m_vertexArray =
            0;


        // =====================================================
        // RAW HALF-RES CLOUD
        // =====================================================

        GLuint m_volumeFramebuffer =
            0;


        GLuint m_volumeCloudTexture =
            0;


        GLuint m_volumeDepthTexture =
            0;


        // =====================================================
        // FILTERED HALF-RES CLOUD
        // =====================================================

        GLuint m_spatialFramebuffer =
            0;


        GLuint m_spatialCloudTexture =
            0;


        GLuint m_spatialDepthTexture =
            0;


        int m_volumeWidth =
            0;


        int m_volumeHeight =
            0;


        // =====================================================
        // FULL-RES RESULT
        // =====================================================

        GLuint m_compositeFramebuffer =
            0;


        GLuint m_compositeTexture =
            0;


        int m_width =
            0;


        int m_height =
            0;


        // =====================================================
        // CONTROLLED FORMATION
        // =====================================================

        CloudFormation m_formation =
            makeTestCumulusFormation();


        bool m_formationLocked =
            false;
    };
}