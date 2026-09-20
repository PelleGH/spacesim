#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct PrimaryStarComponent
    {
        glm::vec3 direction{-0.60f, 0.35f, -0.70f};
        glm::vec3 radiance{20.0f, 19.5f, 18.5f};
    };
}
