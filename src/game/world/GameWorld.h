#pragma once

#include "world/StarSystem.h"
#include "lighting/SceneLighting.h"

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

    enum class PlanetRenderMode
    {
        Distant,
        Orbital,
        Surface
    };

    struct PlanetTransitionState
    {
        int closestPlanetIndex = -1;
        PlanetRenderMode mode = PlanetRenderMode::Distant;

        double distanceToCenter = 0.0;
        double altitude = 0.0;

        double orbitalEnterAltitude = 0.0;
        double orbitalExitAltitude = 0.0;

        double surfaceEnterAltitude = 2000.0;
        double surfaceExitAltitude = 2500.0;
    };

    struct OrbitalCruiseState
    {
        bool active = false;

        double maximumSpeed = 6000.0;
        double currentSpeed = 0.0;

        double slowdownAltitude = 5000.0;
        double minimumAltitude = 1000.0;
    };

    struct AtmosphereState
    {
        bool insideAtmosphere = false;

        double atmosphereHeight = 4000.0;
        float density = 0.0f;
    };

    class GameWorld
    {
    public:
        entt::registry registry;

        entt::entity playerShip = entt::null;

        Camera3D camera{};

        StarSystem starSystem;
        SceneLighting lighting{};

        DVec3 globalPlayerPosition{};
        DVec3 activeBubbleOrigin{};

        int selectedJumpTarget = -1;
        int activePoi = -1;

        TravelMode travelMode = TravelMode::NormalFlight;
        FTLTravelState ftlTravel{};

        PlanetTransitionState planetTransition{};
        OrbitalCruiseState orbitalCruise{};
        AtmosphereState atmosphere{};
    };
}
