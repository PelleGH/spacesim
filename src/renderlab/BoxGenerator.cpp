#include "renderlab/BoxGenerator.h"

namespace SpaceSim
{
    BoxMeshData generateBox(
        float width,
        float height,
        float length)
    {
        BoxMeshData result;

        const float x = width * 0.5f;
        const float y = height * 0.5f;
        const float z = length * 0.5f;

        // We use separate vertices per face so every face has
        // a clean flat normal.

        result.vertices =
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

        for (std::uint32_t face = 0; face < 6; ++face)
        {
            const std::uint32_t i = face * 4;

            result.indices.push_back(i + 0);
            result.indices.push_back(i + 1);
            result.indices.push_back(i + 2);

            result.indices.push_back(i + 0);
            result.indices.push_back(i + 2);
            result.indices.push_back(i + 3);
        }

        return result;
    }
}