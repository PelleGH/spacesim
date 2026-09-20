#include "renderlab/TestEnvironment.h"

#include <glm/vec4.hpp>

#include <vector>

namespace SpaceSim
{
    void fillTestEnvironment(
        GlTextureCube& texture)
    {
        const int size =
            texture.resolution();

        const glm::vec4 colors[6] =
        {
            // +X
            { 4.0f, 0.15f, 0.10f, 1.0f },

            // -X
            { 0.05f, 0.15f, 2.5f, 1.0f },

            // +Y
            { 0.25f, 0.40f, 1.5f, 1.0f },

            // -Y
            { 0.02f, 0.02f, 0.025f, 1.0f },

            // +Z
            { 0.10f, 1.8f, 0.25f, 1.0f },

            // -Z
            { 1.8f, 0.20f, 1.2f, 1.0f }
        };

        std::vector<glm::vec4> pixels(
            static_cast<std::size_t>(size) *
            static_cast<std::size_t>(size));

        for (int face = 0;
             face < 6;
             ++face)
        {
            for (glm::vec4& pixel : pixels)
            {
                pixel =
                    colors[face];
            }

            glTextureSubImage3D(
                texture.id(),
                0,

                0,
                0,
                face,

                size,
                size,
                1,

                GL_RGBA,
                GL_FLOAT,

                pixels.data());
        }
    }
}