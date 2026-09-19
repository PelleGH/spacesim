#pragma once

#include "renderer/MeshVertex.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace SpaceSim
{
    struct MeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;
    };

    struct ModelData
    {
        std::vector<MeshData> meshes;

        glm::vec3 boundsMin{0.0f};
        glm::vec3 boundsMax{0.0f};
    };
}