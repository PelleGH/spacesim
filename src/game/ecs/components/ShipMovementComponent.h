#pragma once

#include <glm/vec3.hpp>

namespace SpaceSim
{
    struct ShipMovementComponent
    {
        // Physical local-flight units.
        glm::dvec3 velocityMetersPerSecond{0.0};

        // Rotation rate around local X/Y/Z axes in radians/sec.
        glm::vec3 angularVelocityLocalRadiansPerSecond{0.0f};

        // -------------------------------------------------------------
        // NORMAL LOCAL FLIGHT
        // -------------------------------------------------------------

        float forwardAccelerationMetersPerSecondSquared = 28.0f;
        float reverseAccelerationMetersPerSecondSquared = 18.0f;
        float strafeAccelerationMetersPerSecondSquared = 16.0f;
        float verticalAccelerationMetersPerSecondSquared = 16.0f;

        float boostAccelerationMultiplier = 2.6f;

        float maxSpeedMetersPerSecond = 180.0f;
        float boostMaxSpeedMetersPerSecond = 420.0f;


        // -------------------------------------------------------------
        // ORBITAL CRUISE
        // -------------------------------------------------------------
        //
        // Orbital cruise is intentionally fictional assisted flight rather
        // than a physical orbit simulation.
        //
        // The floating-origin/local-bubble system already lets the ship move
        // through global double-precision space. These values simply provide
        // a much larger manual-flight envelope.

        // Reaches normal cruise speed in several seconds.
        float orbitalCruiseForwardAccelerationMetersPerSecondSquared =
            8000.0f;

        // Slightly stronger reverse thrust makes slowing down manageable.
        float orbitalCruiseReverseAccelerationMetersPerSecondSquared =
            10000.0f;

        // Still allow useful lateral/radial corrections at cruise speeds.
        float orbitalCruiseStrafeAccelerationMetersPerSecondSquared =
            1500.0f;

        float orbitalCruiseVerticalAccelerationMetersPerSecondSquared =
            1500.0f;

        float orbitalCruiseBoostAccelerationMultiplier =
            1.8f;

        // 50 km/s normal cruise.
        float orbitalCruiseMaxSpeedMetersPerSecond =
            50000.0f;

        // 100 km/s while holding Shift.
        float orbitalCruiseBoostMaxSpeedMetersPerSecond =
            100000.0f;

        // While flight assist is enabled, strongly remove velocity sideways
        // relative to the ship. This lets the high-speed velocity vector bend
        // toward the ship's new heading when the player turns.
        float orbitalCruiseLateralDampingPerSecond =
            3.2f;

        // Forward cruise velocity is preserved much more strongly.
        float orbitalCruiseForwardDampingPerSecond =
            0.12f;

        // When leaving cruise at tens of km/s, bleed down toward the normal
        // flight envelope instead of instantly snapping from 50 km/s to
        // 420 m/s.
        float orbitalCruiseExitDecelerationMetersPerSecondSquared =
            25000.0f;


        // -------------------------------------------------------------
        // ROTATION
        // -------------------------------------------------------------

        float pitchAccelerationRadiansPerSecondSquared = 4.0f;
        float yawAccelerationRadiansPerSecondSquared = 3.6f;
        float rollAccelerationRadiansPerSecondSquared = 5.2f;

        float maxPitchRateRadiansPerSecond = 1.20f;
        float maxYawRateRadiansPerSecond = 1.10f;
        float maxRollRateRadiansPerSecond = 1.80f;

        // Mouse rate requests converge toward the requested angular velocity
        // instead of snapping directly to it.
        float mouseTurnResponse = 10.0f;

        // Flight-assist damping used during ordinary local flight.
        //
        // With assist disabled, translational drift and rotational inertia are
        // preserved.
        float linearDampingPerSecond = 0.90f;
        float angularDampingPerSecond = 4.5f;
    };
}