#pragma once

#include <memory>


namespace SpaceSim
{
    struct PlanetRenderObject;
    struct RenderCamera;


    class AdaptivePlanetSurfaceRenderer
    {
    public:
        AdaptivePlanetSurfaceRenderer();

        ~AdaptivePlanetSurfaceRenderer();


        AdaptivePlanetSurfaceRenderer(
            const AdaptivePlanetSurfaceRenderer&) = delete;


        AdaptivePlanetSurfaceRenderer& operator=(
            const AdaptivePlanetSurfaceRenderer&) = delete;


        // Draws the adaptive cube-sphere when requested by the
        // PlanetRenderObject.
        //
        // Until all six root tiles have reached the GPU, the
        // object's ordinary mesh is drawn as a fallback.
        void drawOrMesh(
            const PlanetRenderObject& planet,
            const RenderCamera& camera);


    private:
        struct Impl;

        std::unique_ptr<Impl> m_impl;
    };
}