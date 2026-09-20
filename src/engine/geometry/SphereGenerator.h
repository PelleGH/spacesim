#pragma once

#include "renderer/MeshVertex.h"

#include <cstdint>
#include <vector>

namespace SpaceSim
{
    struct SphereMeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;
    };

    SphereMeshData generateSphere(
        std::uint32_t horizontalSegments,
        std::uint32_t verticalSegments);
}