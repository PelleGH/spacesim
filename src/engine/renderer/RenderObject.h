#pragma once

#include "renderer/material/PbrMaterial.h"

#include <glm/mat4x4.hpp>

namespace SpaceSim
{
    class GpuMesh;

    struct RenderObject
    {
        const GpuMesh* mesh = nullptr;

        glm::mat4 modelMatrix
        {
            1.0f
        };

        PbrMaterial material;
    };
}