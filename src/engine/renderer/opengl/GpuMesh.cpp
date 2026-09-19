#include "renderer/opengl/GpuMesh.h"

#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
namespace SpaceSim
{
    GpuMesh::GpuMesh(
        const std::vector<MeshVertex>& vertices,
        const std::vector<std::uint32_t>& indices)
    {
        m_indexCount =
            static_cast<GLsizei>(indices.size());

        glCreateVertexArrays(
            1,
            &m_vertexArray);

        glCreateBuffers(
            1,
            &m_vertexBuffer);

        glCreateBuffers(
            1,
            &m_indexBuffer);

        glNamedBufferData(
            m_vertexBuffer,
            static_cast<GLsizeiptr>(
                vertices.size() * sizeof(MeshVertex)),
            vertices.data(),
            GL_STATIC_DRAW);

        glNamedBufferData(
            m_indexBuffer,
            static_cast<GLsizeiptr>(
                indices.size() * sizeof(std::uint32_t)),
            indices.data(),
            GL_STATIC_DRAW);

        glVertexArrayVertexBuffer(
            m_vertexArray,
            0,
            m_vertexBuffer,
            0,
            sizeof(MeshVertex));

        glVertexArrayElementBuffer(
            m_vertexArray,
            m_indexBuffer);

        // Position
        glEnableVertexArrayAttrib(
            m_vertexArray,
            0);

        glVertexArrayAttribFormat(
            m_vertexArray,
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            offsetof(MeshVertex, position));

        glVertexArrayAttribBinding(
            m_vertexArray,
            0,
            0);

        // Normal
        glEnableVertexArrayAttrib(
            m_vertexArray,
            1);

        glVertexArrayAttribFormat(
            m_vertexArray,
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            offsetof(MeshVertex, normal));

        glVertexArrayAttribBinding(
            m_vertexArray,
            1,
            0);

        // Texture coordinates
        glEnableVertexArrayAttrib(
            m_vertexArray,
            2);

        glVertexArrayAttribFormat(
            m_vertexArray,
            2,
            2,
            GL_FLOAT,
            GL_FALSE,
            offsetof(MeshVertex, texCoord));

        glVertexArrayAttribBinding(
            m_vertexArray,
            2,
            0);
    }

    GpuMesh::~GpuMesh()
    {
        if (m_indexBuffer != 0)
        {
            glDeleteBuffers(
                1,
                &m_indexBuffer);
        }

        if (m_vertexBuffer != 0)
        {
            glDeleteBuffers(
                1,
                &m_vertexBuffer);
        }

        if (m_vertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vertexArray);
        }
    }

    void GpuMesh::draw() const
    {
        glBindVertexArray(
            m_vertexArray);

        glDrawElements(
            GL_TRIANGLES,
            m_indexCount,
            GL_UNSIGNED_INT,
            nullptr);

        glBindVertexArray(0);
    }
}