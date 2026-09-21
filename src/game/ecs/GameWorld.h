#pragma once

#include <entt/entity/registry.hpp>
#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct GameWorld
    {
        entt::registry registry;
        float renderInterpolationAlpha = 0.0f;
        // Global position corresponding to local gameplay coordinate (0,0,0).
        // Future floating-origin/bubble rebasing changes this value while local
        // entities remain near zero.
        glm::dvec3 localBubbleOriginMeters{0.0, 0.0, 0.0};

        entt::entity activeCamera = entt::null;
        entt::entity primaryPlanet = entt::null;
        entt::entity primaryStar = entt::null;
        entt::entity playerShip = entt::null;

        glm::dvec3 localToGlobalMeters(const glm::dvec3& localPositionMeters) const
        {
            return localBubbleOriginMeters + localPositionMeters;
        }
    };
}
