#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    // Temporary flight-toy debug geometry with explicit physical dimensions.
    // It exists so speed, camera distance and ship size can be judged against
    // known metre-scale objects before real stations/asteroids are present.
    struct ScaleReferenceComponent
    {
        glm::vec3 sizeMeters{10.0f, 10.0f, 10.0f};
        glm::vec3 baseColor{0.72f, 0.70f, 0.18f};
        float emissiveStrength = 0.35f;
    };
}
