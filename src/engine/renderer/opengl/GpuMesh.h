#pragma once

#include "renderer/MeshVertex.h"

#include <glad/gl.h>

#include <cstdint>
#include <vector>

namespace SpaceSim
{
    class GpuMesh
    {
    public:
        GpuMesh(
            const std::vector<MeshVertex>& vertices,
            const std::vector<std::uint32_t>& indices);

        ~GpuMesh();

        GpuMesh(const GpuMesh&) = delete;
        GpuMesh& operator=(const GpuMesh&) = delete;

        void draw() const;

    private:
        GLuint m_vertexArray = 0;
        GLuint m_vertexBuffer = 0;
        GLuint m_indexBuffer = 0;

        GLsizei m_indexCount = 0;
    };
}