#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace SpaceSim
{
    // Per-frame control intent. This is deliberately independent from SDL so
    // players, AI, autopilot and replay code can all drive the same movement
    // system later.
    struct ShipControlComponent
    {
        // x = strafe right, y = thrust up, z = thrust forward.
        glm::vec3 linearInput{0.0f};

        // x = pitch up, y = yaw right, z = roll right. Keyboard/controller
        // input remains normalized to [-1, 1].
        glm::vec3 angularInput{0.0f};

        // Mouse steering is represented as an angular-rate request so the
        // fixed-step movement remains stable when render FPS changes.
        // x = pitch radians/sec, y = yaw radians/sec.
        glm::vec2 mouseLookRate{0.0f};
        bool mouseSteering = false;

        bool boost = false;
        bool flightAssist = true;
    };
}
