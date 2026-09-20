#include "game/rendering/GameRenderResources.h"

#include "geometry/SphereGenerator.h"
#include "renderer/MeshVertex.h"
#include "renderer/TestEnvironment.h"

#include <cstdint>
#include <vector>

namespace SpaceSim
{
    namespace
    {
        std::unique_ptr<GpuMesh> createUnitBox()
        {
            constexpr float x = 0.5f;
            constexpr float y = 0.5f;
            constexpr float z = 0.5f;

            const std::vector<MeshVertex> vertices =
            {
                // Front +Z
                {{-x, -y,  z}, { 0,  0,  1}, {0, 0}},
                {{ x, -y,  z}, { 0,  0,  1}, {1, 0}},
                {{ x,  y,  z}, { 0,  0,  1}, {1, 1}},
                {{-x,  y,  z}, { 0,  0,  1}, {0, 1}},

                // Back -Z
                {{ x, -y, -z}, { 0,  0, -1}, {0, 0}},
                {{-x, -y, -z}, { 0,  0, -1}, {1, 0}},
                {{-x,  y, -z}, { 0,  0, -1}, {1, 1}},
                {{ x,  y, -z}, { 0,  0, -1}, {0, 1}},

                // Left -X
                {{-x, -y, -z}, {-1,  0,  0}, {0, 0}},
                {{-x, -y,  z}, {-1,  0,  0}, {1, 0}},
                {{-x,  y,  z}, {-1,  0,  0}, {1, 1}},
                {{-x,  y, -z}, {-1,  0,  0}, {0, 1}},

                // Right +X
                {{ x, -y,  z}, { 1,  0,  0}, {0, 0}},
                {{ x, -y, -z}, { 1,  0,  0}, {1, 0}},
                {{ x,  y, -z}, { 1,  0,  0}, {1, 1}},
                {{ x,  y,  z}, { 1,  0,  0}, {0, 1}},

                // Top +Y
                {{-x,  y,  z}, { 0,  1,  0}, {0, 0}},
                {{ x,  y,  z}, { 0,  1,  0}, {1, 0}},
                {{ x,  y, -z}, { 0,  1,  0}, {1, 1}},
                {{-x,  y, -z}, { 0,  1,  0}, {0, 1}},

                // Bottom -Y
                {{-x, -y, -z}, { 0, -1,  0}, {0, 0}},
                {{ x, -y, -z}, { 0, -1,  0}, {1, 0}},
                {{ x, -y,  z}, { 0, -1,  0}, {1, 1}},
                {{-x, -y,  z}, { 0, -1,  0}, {0, 1}},
            };

            std::vector<std::uint32_t> indices;
            indices.reserve(36);
            for (std::uint32_t face = 0; face < 6; ++face)
            {
                const std::uint32_t i = face * 4;
                indices.push_back(i + 0);
                indices.push_back(i + 1);
                indices.push_back(i + 2);
                indices.push_back(i + 0);
                indices.push_back(i + 2);
                indices.push_back(i + 3);
            }

            return std::make_unique<GpuMesh>(vertices, indices);
        }
    }

    GameRenderResources::GameRenderResources(const AtmosphereParameters& atmosphereParameters)
        : m_environmentMap(64)
    {
        fillTestEnvironment(m_environmentMap);
        m_environmentIbl = std::make_unique<EnvironmentIbl>(m_environmentMap);
        m_atmosphereLuts = std::make_unique<AtmosphereLuts>(atmosphereParameters);

        const SphereMeshData sphere = generateSphere(1024, 512);
        m_planetSphere = std::make_unique<GpuMesh>(sphere.vertices, sphere.indices);
        m_placeholderBox = createUnitBox();
    }

    const GpuMesh& GameRenderResources::planetSphere() const
    {
        return *m_planetSphere;
    }

    const GpuMesh& GameRenderResources::placeholderBox() const
    {
        return *m_placeholderBox;
    }

    const GlTextureCube& GameRenderResources::environmentMap() const
    {
        return m_environmentMap;
    }

    const EnvironmentIbl& GameRenderResources::environmentIbl() const
    {
        return *m_environmentIbl;
    }

    const AtmosphereLuts& GameRenderResources::atmosphereLuts() const
    {
        return *m_atmosphereLuts;
    }
}
