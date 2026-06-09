#pragma once

#include <raylib.h>

namespace SpaceSim
{
    struct TransformComponent
    {
        Vector3 position{ 0.0f, 0.0f, 0.0f };
        Quaternion rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
        Vector3 scale{ 1.0f, 1.0f, 1.0f };
    };
}