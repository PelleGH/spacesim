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

        float forwardAccelerationMetersPerSecondSquared = 28.0f;
        float reverseAccelerationMetersPerSecondSquared = 18.0f;
        float strafeAccelerationMetersPerSecondSquared = 16.0f;
        float verticalAccelerationMetersPerSecondSquared = 16.0f;

        float boostAccelerationMultiplier = 2.6f;
        float maxSpeedMetersPerSecond = 180.0f;
        float boostMaxSpeedMetersPerSecond = 420.0f;

        float pitchAccelerationRadiansPerSecondSquared = 4.0f;
        float yawAccelerationRadiansPerSecondSquared = 3.6f;
        float rollAccelerationRadiansPerSecondSquared = 5.2f;

        float maxPitchRateRadiansPerSecond = 1.20f;
        float maxYawRateRadiansPerSecond = 1.10f;
        float maxRollRateRadiansPerSecond = 1.80f;

        // Mouse rate requests converge toward the requested angular velocity
        // instead of snapping directly to it.
        float mouseTurnResponse = 10.0f;

        // Flight-assist damping. With assist disabled, translational drift and
        // rotational inertia are preserved.
        float linearDampingPerSecond = 0.90f;
        float angularDampingPerSecond = 4.5f;
    };
}
