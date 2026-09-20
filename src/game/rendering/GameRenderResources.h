#pragma once

#include "renderer/EnvironmentIbl.h"
#include "renderer/atmosphere/AtmosphereLuts.h"
#include "renderer/atmosphere/AtmosphereParameters.h"
#include "renderer/opengl/GlTextureCube.h"
#include "renderer/opengl/GpuMesh.h"

#include <memory>

namespace SpaceSim
{
    class GameRenderResources
    {
    public:
        explicit GameRenderResources(const AtmosphereParameters& atmosphereParameters);

        const GpuMesh& planetSphere() const;
        const GpuMesh& placeholderBox() const;
        const GlTextureCube& environmentMap() const;
        const EnvironmentIbl& environmentIbl() const;
        const AtmosphereLuts& atmosphereLuts() const;

    private:
        GlTextureCube m_environmentMap;
        std::unique_ptr<EnvironmentIbl> m_environmentIbl;
        std::unique_ptr<GpuMesh> m_planetSphere;
        std::unique_ptr<GpuMesh> m_placeholderBox;
        std::unique_ptr<AtmosphereLuts> m_atmosphereLuts;
    };
}
