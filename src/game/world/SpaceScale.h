#pragma once

namespace SpaceSim::SpaceScale
{
    // Gameplay/simulation uses real physical units. Local TransformComponent
    // positions are metres and ShipMovementComponent velocities are m/s.
    inline constexpr double MetersPerKilometer = 1000.0;

    // Camera-relative nearby rendering currently uses 100 physical metres per
    // renderer unit. The near-body planet representation uses this exact same
    // scale so ship-to-surface distance is physically meaningful.
    inline constexpr float LocalMetersPerRenderUnit = 100.0f;
    inline constexpr float RenderUnitsPerLocalMeter = 1.0f / LocalMetersPerRenderUnit;

    // Far planets use the compact angular-size representation. Once altitude
    // falls below this threshold, RenderSystem switches to a true camera-
    // relative planet center/radius using the local physical scale above.
    inline constexpr double NearBodyTransitionAltitudeMeters = 1500.0 * MetersPerKilometer;

    // Reserved for the later terrain/local-tangent-frame handoff. NearBody can
    // currently be flown all the way to radius=surface, but detailed landing
    // terrain should eventually take over around this altitude.
    inline constexpr double SurfaceTransitionAltitudeMeters = 100.0 * MetersPerKilometer;

    // Distant bodies are rendered at a safe fake distance. Their visual radius
    // is derived from their true radius/distance ratio, preserving angular size.
    inline constexpr float DistantBodyCenterRenderUnits = 220.0f;

    // Keep enough render depth for the fake distant-body layer regardless of
    // the local gameplay camera's physical far clip. Near-body mode expands
    // this dynamically to include the physical planet sphere.
    inline constexpr float MinimumRenderFarPlane = 600.0f;
}
