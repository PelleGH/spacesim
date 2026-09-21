#include "game/systems/LocalBubbleRebaseSystem.h"

#include "game/ecs/components/TransformComponent.h"

#include <glm/geometric.hpp>

namespace SpaceSim
{
    void LocalBubbleRebaseSystem::fixedUpdate(GameWorld& world)
    {
        if (world.playerShip == entt::null ||
            !world.registry.valid(world.playerShip) ||
            !world.registry.all_of<TransformComponent>(world.playerShip))
        {
            return;
        }

        const glm::dvec3 playerLocalMeters =
            world.registry.get<TransformComponent>(world.playerShip).positionMeters;

        if (glm::length(playerLocalMeters) < RebaseDistanceMeters)
        {
            return;
        }

        // Move local zero to the player's current global position. Subtracting
        // the same offset from every local transform preserves all pairwise and
        // global positions while keeping the active gameplay bubble near zero.
        world.localBubbleOriginMeters += playerLocalMeters;

        const auto transformView = world.registry.view<TransformComponent>();
        for (const entt::entity entity : transformView)
        {
            auto& transform = transformView.get<TransformComponent>(entity);
            transform.positionMeters -= playerLocalMeters;
        }
    }
}
