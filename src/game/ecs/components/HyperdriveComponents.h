#pragma once

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include <string>

namespace SpaceSim
{
    // Marks a global-space entity as a selectable hyperdrive destination.
    // arrivalDistanceMeters is measured from the target's global position.
    // For a planet/moon this is normally body radius + desired arrival altitude.
    struct JumpPointComponent
    {
        std::string displayName{"Unnamed jump point"};
        double arrivalDistanceMeters = 100000.0;
    };

    // Discrete player intent. These are edge-triggered requests written by the
    // input system and consumed/cleared by HyperdriveSystem once per frame.
    struct HyperdriveControlComponent
    {
        bool cycleTargetRequested = false;
        bool engageRequested = false;
    };

    enum class HyperdriveState
    {
        Idle,
        Aligning,
        Traveling
    };

    struct HyperdriveComponent
    {
        entt::entity selectedTarget = entt::null;
        entt::entity activeTarget = entt::null;
        HyperdriveState state = HyperdriveState::Idle;

        // Resolved once when a jump begins. The ship then flies through the
        // global coordinate layer toward this exact point rather than being
        // teleported there.
        glm::dvec3 arrivalGlobalMeters{0.0};

        double travelSpeedMetersPerSecond = 0.0;
        double remainingDistanceMeters = 0.0;

        // Prototype tuning. These are intentionally fictional hyperdrive
        // values rather than local-flight physics values.
        double travelAccelerationMetersPerSecondSquared = 2'000'000.0;
        double maximumTravelSpeedMetersPerSecond = 6'000'000.0;
        float alignmentTurnRateRadiansPerSecond = 1.35f;
        float alignmentToleranceRadians = 0.008726646f; // 0.5 degrees
    };
}
