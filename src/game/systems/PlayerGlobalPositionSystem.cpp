#include "systems/PlayerGlobalPositionSystem.h"

#include "components/TransformComponent.h"
#include "world/GameWorld.h"
#include "world/SpaceCoordinates.h"

namespace SpaceSim
{
    void PlayerGlobalPositionSystem::update(GameWorld& world)
    {
        if (world.travelMode != TravelMode::NormalFlight)
        {
            return;
        }

        if (world.playerShip == entt::null ||
            !world.registry.valid(world.playerShip) ||
            !world.registry.all_of<TransformComponent>(world.playerShip))
        {
            return;
        }

        const auto& transform =
            world.registry.get<TransformComponent>(world.playerShip);

        world.globalPlayerPosition =
            LocalToGlobalPosition(world, transform.position);
    }
}
