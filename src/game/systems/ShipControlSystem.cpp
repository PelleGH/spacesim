#include "systems/ShipControlSystem.h"

#include "components/PlayerControlledComponent.h"
#include "components/ShipFlightComponent.h"
#include "components/TransformComponent.h"

#include <raylib.h>
#include <raymath.h>
#include <cmath>

namespace SpaceSim
{
    static void ApplyShipPreset(ShipFlightComponent& flight, ShipPreset preset)
{
    flight.preset = preset;

    switch (preset)
    {
    case ShipPreset::Light:
        flight.forwardAcceleration = 35.0f;
        flight.strafeAcceleration = 40.0f;
        flight.verticalAcceleration = 40.0f;
        flight.maxSpeed = 45.0f;

        flight.pitchAcceleration = 160.0f;
        flight.yawAcceleration = 160.0f;
        flight.rollAcceleration = 180.0f;
        flight.maxAngularSpeed = 120.0f;
        flight.turnResponse = 14.0f;

        flight.linearDamping = 1.4f;
        flight.angularDamping = 9.0f;
        break;

    case ShipPreset::Medium:
        flight.forwardAcceleration = 28.0f;
        flight.strafeAcceleration = 26.0f;
        flight.verticalAcceleration = 26.0f;
        flight.maxSpeed = 55.0f;

        flight.pitchAcceleration = 120.0f;
        flight.yawAcceleration = 120.0f;
        flight.rollAcceleration = 140.0f;
        flight.maxAngularSpeed = 90.0f;
        flight.turnResponse = 10.0f;

        flight.linearDamping = 1.0f;
        flight.angularDamping = 7.0f;
        break;

    case ShipPreset::Heavy:
        flight.forwardAcceleration = 20.0f;
        flight.strafeAcceleration = 14.0f;
        flight.verticalAcceleration = 14.0f;
        flight.maxSpeed = 65.0f;

        flight.pitchAcceleration = 75.0f;
        flight.yawAcceleration = 75.0f;
        flight.rollAcceleration = 90.0f;
        flight.maxAngularSpeed = 55.0f;
        flight.turnResponse = 6.0f;

        flight.linearDamping = 0.7f;
        flight.angularDamping = 5.0f;
        break;
    }
}
void ShipControlSystem::update(GameWorld& world, float dt)
{
    auto view = world.registry.view<
        TransformComponent,
        ShipFlightComponent,
        PlayerControlledComponent
    >();

    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& flight = view.get<ShipFlightComponent>(entity);
        if (IsKeyPressed(KEY_ONE))
        {
            ApplyShipPreset(flight, ShipPreset::Light);
        }

        if (IsKeyPressed(KEY_TWO))
        {
            ApplyShipPreset(flight, ShipPreset::Medium);
        }

        if (IsKeyPressed(KEY_THREE))
        {
            ApplyShipPreset(flight, ShipPreset::Heavy);
        }
        if (IsKeyPressed(KEY_X))
        {
            flight.flightAssist = !flight.flightAssist;
        }

        Vector3 inputMove{ 0.0f, 0.0f, 0.0f };

        if (IsKeyDown(KEY_D)) inputMove.x -= 1.0f;
        if (IsKeyDown(KEY_A)) inputMove.x += 1.0f;

        if (IsKeyDown(KEY_SPACE)) inputMove.y += 1.0f;
        if (IsKeyDown(KEY_LEFT_CONTROL)) inputMove.y -= 1.0f;

        // W/S now control throttle, not direct forward thrust.
        if (IsKeyPressed(KEY_T))
        {
            flight.holdThrustMode = !flight.holdThrustMode;
            flight.throttle = 0.0f;
        }

        // Toggle throttle mode:
        // W/S adjust throttle and it stays there.
        if (!flight.holdThrustMode)
        {
            if (IsKeyDown(KEY_W))
            {
                flight.throttle += flight.throttleChangeSpeed * dt;
            }

            if (IsKeyDown(KEY_S))
            {
                flight.throttle -= flight.throttleChangeSpeed * dt;
            }

            flight.throttle = Clamp(flight.throttle, -1.0f, 1.0f);

            if (IsKeyPressed(KEY_Z))
            {
                flight.throttle = 0.0f;
            }
        }
        // Hold thrust mode:
        // W/S only apply thrust while held.
        else
        {
            flight.throttle = 0.0f;

            if (IsKeyDown(KEY_W))
            {
                flight.throttle += 1.0f;
            }

            if (IsKeyDown(KEY_S))
            {
                flight.throttle -= 1.0f;
            }
        }
        if (Vector3Length(inputMove) > 0.0f)
        {
            inputMove = Vector3Normalize(inputMove);
        }

        Vector3 localRight = Vector3RotateByQuaternion(Vector3{ 1.0f, 0.0f, 0.0f }, transform.rotation);
        Vector3 localUp = Vector3RotateByQuaternion(Vector3{ 0.0f, 1.0f, 0.0f }, transform.rotation);
        Vector3 localForward = Vector3RotateByQuaternion(Vector3{ 0.0f, 0.0f, 1.0f }, transform.rotation);

        Vector3 acceleration{ 0.0f, 0.0f, 0.0f };

        acceleration = Vector3Add(
            acceleration,
            Vector3Scale(localRight, inputMove.x * flight.strafeAcceleration)
        );

        acceleration = Vector3Add(
            acceleration,
            Vector3Scale(localUp, inputMove.y * flight.verticalAcceleration)
        );

        acceleration = Vector3Add(
            acceleration,
            Vector3Scale(localForward, flight.throttle * flight.forwardAcceleration)
        );

        flight.velocity = Vector3Add(
            flight.velocity,
            Vector3Scale(acceleration, dt)
        );

        float speed = Vector3Length(flight.velocity);

        if (speed > flight.maxSpeed)
        {
            flight.velocity = Vector3Scale(
                Vector3Normalize(flight.velocity),
                flight.maxSpeed
            );
        }

        Vector2 mouseDelta = GetMouseDelta();

        m_virtualStick.x += mouseDelta.x * flight.mouseSensitivity;
        m_virtualStick.y += mouseDelta.y * flight.mouseSensitivity;

        float stickLength = Vector2Length(m_virtualStick);

        if (stickLength > m_controlRadius)
        {
            m_virtualStick = Vector2Scale(
                Vector2Normalize(m_virtualStick),
                m_controlRadius
            );
        }
        if (IsKeyPressed(KEY_R))
        {
            m_virtualStick = Vector2{ 0.0f, 0.0f };
        }

/*         if (Vector2Length(mouseDelta) < 0.01f)
        {
            float recenterAmount = Clamp(flight.mouseRecentering * dt, 0.0f, 1.0f);

            m_virtualStick = Vector2Lerp(
                m_virtualStick,
                Vector2{ 0.0f, 0.0f },
                recenterAmount
            );
        } */

        Vector2 turnInput{
            m_virtualStick.x / m_controlRadius,
            m_virtualStick.y / m_controlRadius
        };
        auto applyDeadzoneCurve = [](float value, float deadzone, float curve)
        {
            const float sign = value < 0.0f ? -1.0f : 1.0f;
            float amount = std::fabs(value);

            if (amount <= deadzone)
            {
                return 0.0f;
            }

            amount = (amount - deadzone) / (1.0f - deadzone);
            amount = Clamp(amount, 0.0f, 1.0f);
            amount = std::pow(amount, curve);

            return sign * amount;
        };

        const float yawInput = applyDeadzoneCurve(
            turnInput.x,
            flight.mouseDeadzone,
            flight.mouseInputCurve
        );

        const float pitchInput = applyDeadzoneCurve(
            turnInput.y,
            flight.mouseDeadzone,
            flight.mouseInputCurve
        );

        const float desiredYawRate = -yawInput * flight.maxAngularSpeed;
        const float desiredPitchRate = pitchInput * flight.maxAngularSpeed;

        flight.angularVelocity.y = Lerp(
            flight.angularVelocity.y,
            desiredYawRate,
            Clamp(flight.turnResponse * dt, 0.0f, 1.0f)
        );

        flight.angularVelocity.x = Lerp(
            flight.angularVelocity.x,
            desiredPitchRate,
            Clamp(flight.turnResponse * dt, 0.0f, 1.0f)
        );

        Vector3 angularAcceleration{ 0.0f, 0.0f, 0.0f };

        if (IsKeyDown(KEY_Q)) angularAcceleration.z -= flight.rollAcceleration;
        if (IsKeyDown(KEY_E)) angularAcceleration.z += flight.rollAcceleration;

        flight.angularVelocity.z += angularAcceleration.z * dt;
        flight.angularVelocity.z = Clamp(
            flight.angularVelocity.z,
            -flight.maxAngularSpeed,
            flight.maxAngularSpeed
        );


        const bool noTranslationIntent =
            std::fabs(flight.throttle) < 0.01f &&
            !IsKeyDown(KEY_W) &&
            !IsKeyDown(KEY_S) &&
            !IsKeyDown(KEY_A) &&
            !IsKeyDown(KEY_D) &&
            !IsKeyDown(KEY_SPACE) &&
            !IsKeyDown(KEY_LEFT_CONTROL);

        // Flight Assist only controls linear drift braking.
        if (flight.flightAssist && noTranslationIntent)
        {
            const float dampingAmount = Clamp(flight.linearDamping * dt, 0.0f, 1.0f);

            flight.velocity = Vector3Lerp(
                flight.velocity,
                Vector3{ 0.0f, 0.0f, 0.0f },
                dampingAmount
            );
        }

        if (!IsKeyDown(KEY_Q) && !IsKeyDown(KEY_E))
        {
            const float angularDampingAmount = Clamp(flight.angularDamping * dt, 0.0f, 1.0f);

            flight.angularVelocity.z = Lerp(
                flight.angularVelocity.z,
                0.0f,
                angularDampingAmount
            );
        }
        

        transform.position = Vector3Add(
            transform.position,
            Vector3Scale(flight.velocity, dt)
        );

        float pitchRadians = DEG2RAD * flight.angularVelocity.x * dt;
        float yawRadians   = DEG2RAD * flight.angularVelocity.y * dt;
        float rollRadians  = DEG2RAD * flight.angularVelocity.z * dt;

        Quaternion pitchRotation = QuaternionFromAxisAngle(Vector3{ 1.0f, 0.0f, 0.0f }, pitchRadians);
        Quaternion yawRotation   = QuaternionFromAxisAngle(Vector3{ 0.0f, 1.0f, 0.0f }, yawRadians);
        Quaternion rollRotation  = QuaternionFromAxisAngle(Vector3{ 0.0f, 0.0f, 1.0f }, rollRadians);

        Quaternion deltaRotation = QuaternionMultiply(
            QuaternionMultiply(yawRotation, pitchRotation),
            rollRotation
        );

        transform.rotation = QuaternionMultiply(transform.rotation, deltaRotation);
        transform.rotation = QuaternionNormalize(transform.rotation);
    }
}
}