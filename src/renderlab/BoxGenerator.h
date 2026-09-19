#pragma once

#include "renderer/MeshVertex.h"

#include <cstdint>
#include <vector>

namespace SpaceSim
{
    struct BoxMeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;
    };

    BoxMeshData generateBox(
        float width,
        float height,
        float length);
}