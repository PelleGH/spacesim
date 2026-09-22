#pragma once

#include "renderer/atmosphere/AtmosphereParameters.h"
#include "renderer/planet/PlanetMaterial.h"

#include <cstdint>
#include <glm/vec3.hpp>

namespace SpaceSim
{
    enum class PlanetPreset
    {
        EarthLikeOceanWorld
    };

    struct PlanetGenerationRequest
    {
        PlanetPreset preset = PlanetPreset::EarthLikeOceanWorld;
        std::uint32_t seed = 1u;
    };

    struct AtmosphereComposition
    {
        // Volume / mole fractions. The generator normalizes these when deriving
        // molecular properties, so they do not have to add up to exactly 1.0.
        double nitrogen = 0.0;
        double oxygen = 0.0;
        double argon = 0.0;
        double carbonDioxide = 0.0;
        double methane = 0.0;
        double hydrogen = 0.0;
        double helium = 0.0;
    };

    struct AerosolDefinition
    {
        // Kept separate from the molecular gas mixture. This is a relative
        // artistic/physical density for dust, salt, haze, etc. Earth-like = 1.
        float relativeDensity = 1.0f;
        float scaleHeightKm = 1.2f;
        float anisotropy = 0.8f;
    };

    struct CloudDefinition
    {
        // Not rendered by the current game yet. Keeping this separate now
        // prevents clouds from becoming another AtmosphereType-style flag.
        float coverage = 0.55f;
        float baseAltitudeKm = 1.5f;
        float thicknessKm = 3.0f;
        float opticalThickness = 1.0f;
    };

    struct AtmosphereDefinition
    {
        double surfacePressurePascals = 0.0;
        AtmosphereComposition composition{};
        AerosolDefinition aerosols{};

        // Current renderer has an ozone-specific absorption profile. This is a
        // temporary bridge until absorption is generalized per species.
        float ozoneRelativeStrength = 0.0f;
        float ozoneCenterHeightKm = 25.0f;
        float ozoneHalfWidthKm = 15.0f;
    };

    struct PlanetPhysicalProperties
    {
        double radiusMeters = 0.0;
        double bulkDensityKgPerCubicMeter = 0.0;

        // Derived from radius + bulk density by PlanetGenerator.
        double massKg = 0.0;
        double surfaceGravityMetersPerSecondSquared = 0.0;

        double rotationPeriodSeconds = 0.0;
        double axialTiltRadians = 0.0;

        double meanSurfaceTemperatureKelvin = 0.0;
        double bondAlbedo = 0.0;
    };

    struct PlanetSurfaceDefinition
    {
        bool hasLiquidOcean = false;

        // This is a generation target, not a guarantee. The current procedural
        // height field uses it to choose an approximate sea-level quantile.
        float targetOceanCoverage = 0.0f;

        // Resolved threshold in the current procedural height field. Values
        // below this threshold are ocean. Keeping it in the surface definition
        // means future gameplay queries do not need to reverse-engineer a
        // renderer material.
        float seaLevelFieldValue = 0.50f;

        // Procedural shaping controls. These are deliberately not presented as
        // fundamental physics; they are knobs for the current terrain model.
        float continentScale = 2.4f;
        float detailScale = 10.0f;
        float coastWidth = 0.018f;
        float terrainReliefScale = 1.0f;
    };

    struct PlanetAppearanceDefinition
    {
        glm::vec3 deepOceanColor{0.006f, 0.022f, 0.055f};
        glm::vec3 shallowOceanColor{0.020f, 0.100f, 0.145f};
        glm::vec3 lowLandColor{0.08f, 0.20f, 0.055f};
        glm::vec3 highLandColor{0.38f, 0.30f, 0.17f};

        float oceanRoughness = 0.10f;
        float landRoughness = 0.82f;

        float oceanWaveScale = 850.0f;
        float oceanWaveStrength = 0.18f;
        float oceanWaveSpeed = 0.65f;

        // Ground albedo used by the atmosphere multiple-scattering model.
        glm::vec3 atmosphereGroundAlbedo{0.10f, 0.14f, 0.18f};
    };

    struct ResolvedPlanet
    {
        std::uint32_t masterSeed = 1u;
        std::uint32_t terrainSeed = 1u;
        std::uint32_t atmosphereSeed = 1u;
        std::uint32_t cloudSeed = 1u;
        std::uint32_t oceanSeed = 1u;

        PlanetPhysicalProperties physical{};
        AtmosphereDefinition atmosphere{};
        CloudDefinition clouds{};
        PlanetSurfaceDefinition surface{};
        PlanetAppearanceDefinition appearance{};
    };

    [[nodiscard]] ResolvedPlanet generatePlanet(
        const PlanetGenerationRequest& request);

    // Derivation boundary: renderer-facing values come from the resolved
    // planet. The renderers never need to branch on PlanetPreset.
    [[nodiscard]] AtmosphereParameters deriveAtmosphereParameters(
        const ResolvedPlanet& planet);

    [[nodiscard]] PlanetMaterial derivePlanetMaterial(
        const ResolvedPlanet& planet);
}
