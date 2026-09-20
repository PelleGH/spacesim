#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    // Large-scale star-system position in physical metres. This is separate
    // from local gameplay TransformComponent coordinates so astronomical
    // placement never has to be fed directly to float rendering/physics.
    struct GlobalPositionComponent
    {
        glm::dvec3 positionMeters{0.0, 0.0, 0.0};
    };
}
