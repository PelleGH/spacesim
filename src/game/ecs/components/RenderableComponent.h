#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    enum class RenderMeshKind
    {
        PlaceholderShip
    };

    // Renderer-facing game data only. GPU objects remain owned by the renderer
    // resource layer and are resolved from RenderMeshKind during extraction.
    struct RenderableComponent
    {
        RenderMeshKind mesh = RenderMeshKind::PlaceholderShip;
        glm::vec3 baseColor{0.34f, 0.38f, 0.44f};
        float metallic = 0.72f;
        float roughness = 0.32f;
    };
}
