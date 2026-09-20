#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace SpaceSim
{
    // Local gameplay transform. One position unit is exactly one metre.
    // Double precision keeps the active bubble comfortable even before origin
    // rebasing is added. Scale is dimensionless.
    struct TransformComponent
    {
        glm::dvec3 positionMeters{0.0, 0.0, 0.0};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
    };
}
