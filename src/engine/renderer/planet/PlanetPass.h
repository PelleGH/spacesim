#pragma once

#include "renderer/RenderCamera.h"

#include "renderer/atmosphere/AtmosphereInstance.h"

#include "renderer/lighting/DirectionalLight.h"

#include "renderer/opengl/GlShader.h"

#include "renderer/planet/PlanetRenderObject.h"
#include "renderer/planet/AdaptivePlanetSurfaceRenderer.h"
#include <glad/gl.h>

#include <memory>
#include <vector>


namespace SpaceSim
{
    class GpuMesh;


    class PlanetPass
    {
    public:
        PlanetPass();

        ~PlanetPass();


        PlanetPass(
            const PlanetPass&) = delete;


        PlanetPass& operator=(
            const PlanetPass&) = delete;


        void render(
            const RenderCamera& camera,
            float aspectRatio,
            const DirectionalLight& sun,
            const std::vector<PlanetRenderObject>& planets,
            const AtmosphereInstance* atmosphere,
            GLuint skyReflectionTexture);


    private:
        AdaptivePlanetSurfaceRenderer m_adaptiveTerrain;
        // Normal whole-planet renderer.
        GlShader m_shader;


        // Near-surface ocean renderer.
        //
        // This has its own lightweight fragment shader so the
        // local water does not pay for the general planet path.
        GlShader m_oceanPatchShader;


        // Concentric stitched grids in kilometres, from 4 m cells outward.
        //
        // ocean_patch.vert bends this grid onto the planet
        // underneath the camera and applies Gerstner waves.
        std::unique_ptr<GpuMesh> m_oceanPatchMesh;

        glm::vec3 m_anchorRight{};
    };
}