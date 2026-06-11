#include "systems/BubbleSystem.h"

#include "components/LocalObjectComponent.h"
#include "components/RenderableComponent.h"
#include "components/TransformComponent.h"
#include "world/SpaceCoordinates.h"

namespace SpaceSim
{
    namespace
    {
        constexpr double BubbleRadiusGlobal = 5000.0;
    }

    bool BubbleSystem::shouldBecomeLocal(const GlobalObject& object) const
    {
        return object.type == GlobalObjectType::Satellite;
    }

    bool BubbleSystem::isInsideBubble(const GameWorld& world, const GlobalObject& object) const
    {
        DVec3 relativeToPlayer = object.position - world.globalPlayerPosition;
        return Length(relativeToPlayer) <= BubbleRadiusGlobal;
    }

    entt::entity BubbleSystem::findLocalEntity(GameWorld& world, const std::string& globalId)
    {
        auto view = world.registry.view<LocalObjectComponent>();

        for (auto entity : view)
        {
            const auto& localObject = view.get<LocalObjectComponent>(entity);

            if (localObject.globalId == globalId)
            {
                return entity;
            }
        }

        return entt::null;
    }

    void BubbleSystem::spawnLocalEntity(GameWorld& world, const GlobalObject& object)
    {
        entt::entity entity = world.registry.create();

        TransformComponent transform{};
        transform.position = GlobalToLocalPosition(world, object.position);
        transform.scale = Vector3{ 1.0f, 1.0f, 1.0f };

        world.registry.emplace<TransformComponent>(entity, transform);
        world.registry.emplace<LocalObjectComponent>(entity, LocalObjectComponent{ object.id });

        RenderableComponent renderable{};
        renderable.type = RenderableType::Satellite;
        renderable.color = object.color;

        world.registry.emplace<RenderableComponent>(entity, renderable);
    }

    void BubbleSystem::destroyAllLocalObjects(GameWorld& world)
    {
        auto view = world.registry.view<LocalObjectComponent>();

        for (auto entity : view)
        {
            world.registry.destroy(entity);
        }
    }

    void BubbleSystem::update(GameWorld& world)
    {
        if (world.travelMode == TravelMode::FTLTravel)
        {
            destroyAllLocalObjects(world);
            return;
        }

        for (const GlobalObject& object : world.starSystem.objects)
        {
            if (!shouldBecomeLocal(object))
            {
                continue;
            }

            const bool insideBubble = isInsideBubble(world, object);
            entt::entity existingEntity = findLocalEntity(world, object.id);

            if (insideBubble)
            {
                if (existingEntity == entt::null)
                {
                    spawnLocalEntity(world, object);
                }
                else
                {
                    auto& transform = world.registry.get<TransformComponent>(existingEntity);
                    transform.position = GlobalToLocalPosition(world, object.position);
                }
            }
            else
            {
                if (existingEntity != entt::null)
                {
                    world.registry.destroy(existingEntity);
                }
            }
        }
    }
}