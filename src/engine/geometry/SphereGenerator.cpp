#include "geometry/SphereGenerator.h"

#include <glm/geometric.hpp>
#include "renderer/MeshVertex.h"
#include <cmath>
#include <numbers>

namespace SpaceSim
{
    SphereMeshData generateSphere(
        std::uint32_t horizontalSegments,
        std::uint32_t verticalSegments)
    {
        SphereMeshData result;

        const float pi =
            std::numbers::pi_v<float>;

        for (std::uint32_t y = 0;
             y <= verticalSegments;
             ++y)
        {
            const float v =
                static_cast<float>(y) /
                static_cast<float>(verticalSegments);

            const float phi =
                v * pi;

            for (std::uint32_t x = 0;
                 x <= horizontalSegments;
                 ++x)
            {
                const float u =
                    static_cast<float>(x) /
                    static_cast<float>(horizontalSegments);

                const float theta =
                    u * 2.0f * pi;

                const float sinPhi =
                    std::sin(phi);

                glm::vec3 position
                {
                    sinPhi * std::cos(theta),
                    std::cos(phi),
                    sinPhi * std::sin(theta)
                };

                MeshVertex vertex;

                vertex.position = position;
                vertex.normal =
                    glm::normalize(position);

                vertex.texCoord =
                    glm::vec2(u, v);

                result.vertices.push_back(
                    vertex);
            }
        }

        const std::uint32_t rowLength =
            horizontalSegments + 1;

        for (std::uint32_t y = 0;
             y < verticalSegments;
             ++y)
        {
            for (std::uint32_t x = 0;
                 x < horizontalSegments;
                 ++x)
            {
                const std::uint32_t topLeft =
                    y * rowLength + x;

                const std::uint32_t bottomLeft =
                    (y + 1) * rowLength + x;

                const std::uint32_t topRight =
                    topLeft + 1;

                const std::uint32_t bottomRight =
                    bottomLeft + 1;

                result.indices.push_back(topLeft);
                result.indices.push_back(bottomLeft);
                result.indices.push_back(topRight);

                result.indices.push_back(topRight);
                result.indices.push_back(bottomLeft);
                result.indices.push_back(bottomRight);
            }
        }

        return result;
    }
}