#pragma once

#include <raylib.h>

namespace SpaceSim
{
    enum class ShipPreset
    {
        Light,
        Medium,
        Heavy
    };

    struct ShipFlightComponent
    {
        ShipPreset preset = ShipPreset::Light;

        Vector3 velocity{ 0.0f, 0.0f, 0.0f };
        Vector3 angularVelocity{ 0.0f, 0.0f, 0.0f };

        float throttle = 0.0f;
        float throttleChangeSpeed = 1.5f;
        bool holdThrustMode = false;

        // Light ship defaults.
        float forwardAcceleration = 35.0f;
        float strafeAcceleration = 40.0f;
        float verticalAcceleration = 40.0f;

        float maxSpeed = 45.0f;

        float pitchAcceleration = 160.0f;
        float yawAcceleration = 160.0f;
        float rollAcceleration = 180.0f;

        float maxAngularSpeed = 120.0f;

        float mouseSensitivity = 1.0f;
        //float mouseRecentering = 4.0f;
        float mouseDeadzone = 0.12f;
        float mouseInputCurve = 1.6f;
        float turnResponse = 14.0f;

        float linearDamping = 1.4f;
        float angularDamping = 9.0f;

        bool flightAssist = true;
    };
}