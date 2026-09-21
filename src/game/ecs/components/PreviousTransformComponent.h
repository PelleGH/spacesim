#pragma once

#include "game/ecs/components/TransformComponent.h"

#include <entt/entity/registry.hpp>

#include <algorithm>

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace SpaceSim
{
    // Stores the previous fixed-simulation transform for entities whose
    // presentation should be interpolated between physics ticks.
    struct PreviousTransformComponent
    {
        TransformComponent transform;
    };

    inline TransformComponent interpolatedTransform(
        const entt::registry& registry,
        entt::entity entity,
        float alpha)
    {
        const auto& current =
            registry.get<TransformComponent>(entity);

        const auto* previous =
            registry.try_get<PreviousTransformComponent>(entity);

        // Entities without a previous transform are rendered normally.
        if (previous == nullptr)
        {
            return current;
        }

        const float t =
            std::clamp(alpha, 0.0f, 1.0f);

        TransformComponent result =
            current;

        result.positionMeters =
            glm::mix(
                previous->transform.positionMeters,
                current.positionMeters,
                static_cast<double>(t));

        result.rotation =
            glm::normalize(
                glm::slerp(
                    previous->transform.rotation,
                    current.rotation,
                    t));

        result.scale =
            glm::mix(
                previous->transform.scale,
                current.scale,
                t);

        return result;
    }
}