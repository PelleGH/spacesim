#include "systems/StarSystemSystem.h"

#include <cmath>

namespace SpaceSim
{
    void StarSystemSystem::update(GameWorld& world, float dt)
    {
        for (auto& object : world.starSystem.objects)
        {
            if (!object.hasOrbit)
            {
                continue;
            }

            object.orbit.angle += object.orbit.angularSpeed * dt;
        }

        for (auto& object : world.starSystem.objects)
        {
            if (!object.hasOrbit)
            {
                continue;
            }

            if (object.orbit.parentIndex < 0 ||
                object.orbit.parentIndex >= static_cast<int>(world.starSystem.objects.size()))
            {
                continue;
            }

            const GlobalObject& parent = world.starSystem.objects[object.orbit.parentIndex];

            const double angle = object.orbit.angle;
            const double radius = object.orbit.radius;

            object.position = {
                parent.position.x + std::cos(angle) * radius,
                parent.position.y,
                parent.position.z + std::sin(angle) * radius
            };
        }
    }
}