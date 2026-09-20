#pragma once

namespace SpaceSim::SpaceScale
{
    // Gameplay/simulation uses real physical units. Local TransformComponent
    // positions are metres and ShipMovementComponent velocities are m/s.
    inline constexpr double MetersPerKilometer = 1000.0;

    // The OpenGL renderer does not need to use metres directly. Keeping nearby
    // gameplay at 100 m per render unit gives the existing planet/atmosphere
    // renderer comfortable float magnitudes while preserving local scale.
    inline constexpr float LocalMetersPerRenderUnit = 100.0f;
    inline constexpr float RenderUnitsPerLocalMeter = 1.0f / LocalMetersPerRenderUnit;

    // Distant bodies are rendered at a safe fake distance. Their visual radius
    // is derived from their true radius/distance ratio, preserving angular size.
    inline constexpr float DistantBodyCenterRenderUnits = 220.0f;

    // Keep enough render depth for the fake distant-body layer regardless of
    // the local gameplay camera's physical far clip.
    inline constexpr float MinimumRenderFarPlane = 600.0f;
}
