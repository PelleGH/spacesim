#pragma once

#include "GameWorld.h"

#include <raylib.h>

namespace SpaceSim
{
    constexpr double LocalToGlobalScale = 0.001;

    inline Vector3 GlobalToLocalPosition(const GameWorld& world, DVec3 globalPosition)
    {
        DVec3 relative = globalPosition - world.activeBubbleOrigin;

        return Vector3{
            static_cast<float>(relative.x / LocalToGlobalScale),
            static_cast<float>(relative.y / LocalToGlobalScale),
            static_cast<float>(relative.z / LocalToGlobalScale)
        };
    }

    inline DVec3 LocalToGlobalPosition(const GameWorld& world, Vector3 localPosition)
    {
        return DVec3{
            world.activeBubbleOrigin.x + localPosition.x * LocalToGlobalScale,
            world.activeBubbleOrigin.y + localPosition.y * LocalToGlobalScale,
            world.activeBubbleOrigin.z + localPosition.z * LocalToGlobalScale
        };
    }
}