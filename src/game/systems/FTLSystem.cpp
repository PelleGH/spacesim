#include "systems/FTLSystem.h"

#include "components/ShipFlightComponent.h"
#include "components/TransformComponent.h"

#include <raylib.h>

namespace SpaceSim
{
    void FTLSystem::update(GameWorld& world, float dt)
    {
        (void)dt;

        if (IsKeyPressed(KEY_TAB))
        {
            selectNextJumpTarget(world);
        }

        if (IsKeyPressed(KEY_J))
        {
            jumpToSelectedTarget(world);
        }
    }

    void FTLSystem::selectNextJumpTarget(GameWorld& world)
    {
        if (world.starSystem.objects.empty())
        {
            world.selectedJumpTarget = -1;
            return;
        }

        int startIndex = world.selectedJumpTarget;

        for (int i = 0; i < static_cast<int>(world.starSystem.objects.size()); ++i)
        {
            startIndex = (startIndex + 1) % static_cast<int>(world.starSystem.objects.size());

            if (world.starSystem.objects[startIndex].isJumpTarget)
            {
                world.selectedJumpTarget = startIndex;
                return;
            }
        }

        world.selectedJumpTarget = -1;
    }

    void FTLSystem::jumpToSelectedTarget(GameWorld& world)
    {
        if (world.selectedJumpTarget < 0 ||
            world.selectedJumpTarget >= static_cast<int>(world.starSystem.objects.size()))
        {
            return;
        }

        GlobalObject& target = world.starSystem.objects[world.selectedJumpTarget];

        if (!target.isJumpTarget)
        {
            return;
        }

        world.activeBubbleOrigin = target.position;
        world.globalPlayerPosition = target.position;
        world.activePoi = world.selectedJumpTarget;

        auto& transform = world.registry.get<TransformComponent>(world.playerShip);
        auto& flight = world.registry.get<ShipFlightComponent>(world.playerShip);

        transform.position = Vector3{ 0.0f, 0.0f, -300.0f };

        flight.velocity = Vector3{ 0.0f, 0.0f, 0.0f };
        flight.angularVelocity = Vector3{ 0.0f, 0.0f, 0.0f };
        flight.throttle = 0.0f;
    }
}