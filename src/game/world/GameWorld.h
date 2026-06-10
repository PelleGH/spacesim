#pragma once

#include "world/StarSystem.h"

#include <entt/entt.hpp>
#include <raylib.h>

namespace SpaceSim
{
    enum class TravelMode
    {
        NormalFlight,
        FTLTravel
    };

    struct FTLTravelState
    {
        int targetIndex = -1;

        DVec3 start{};
        DVec3 destination{};

        double speed = 250000.0;
        double arrivalDistance = 1000.0;

        float chargeTime = 1.25f;
        float chargeTimer = 0.0f;

        float alignSpeed = 2.5f;
    };

    class GameWorld
    {
    public:
        entt::registry registry;

        entt::entity playerShip = entt::null;

        Camera3D camera{};

        StarSystem starSystem;

        DVec3 globalPlayerPosition{};
        DVec3 activeBubbleOrigin{};

        int selectedJumpTarget = -1;
        int activePoi = -1;

        TravelMode travelMode = TravelMode::NormalFlight;
        FTLTravelState ftlTravel{};
    };
}