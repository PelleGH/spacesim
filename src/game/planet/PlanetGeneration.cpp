#include "game/planet/PlanetGeneration.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace SpaceSim
{
    namespace
    {
        constexpr double GravitationalConstant = 6.67430e-11;
        constexpr double UniversalGasConstant = 8.31446261815324;

        constexpr double EarthPressurePascals = 101325.0;
        constexpr double EarthTemperatureKelvin = 288.15;

        std::uint32_t mixSeed(std::uint32_t value, std::uint32_t salt)
        {
            std::uint32_t x = value ^ salt;
            x ^= x >> 16u;
            x *= 0x7feb352du;
            x ^= x >> 15u;
            x *= 0x846ca68bu;
            x ^= x >> 16u;
            return x == 0u ? 1u : x;
        }

        double compositionTotal(const AtmosphereComposition& gas)
        {
            return
                gas.nitrogen +
                gas.oxygen +
                gas.argon +
                gas.carbonDioxide +
                gas.methane +
                gas.hydrogen +
                gas.helium;
        }

        double meanMolarMassKgPerMol(const AtmosphereComposition& gas)
        {
            const double total = std::max(compositionTotal(gas), 1.0e-12);

            const double weighted =
                gas.nitrogen * 0.0280134 +
                gas.oxygen * 0.0319988 +
                gas.argon * 0.0399480 +
                gas.carbonDioxide * 0.0440095 +
                gas.methane * 0.0160425 +
                gas.hydrogen * 0.00201588 +
                gas.helium * 0.004002602;

            return weighted / total;
        }

        double molecularScatteringStrength(const AtmosphereComposition& gas)
        {
            // A deliberately first-order relative model. It preserves the
            // renderer's wavelength-dependent Rayleigh color while allowing
            // different gas mixtures to alter the overall molecular optical
            // depth. These are not spectroscopy-grade coefficients.
            const double total = std::max(compositionTotal(gas), 1.0e-12);

            const double weighted =
                gas.nitrogen * 1.00 +
                gas.oxygen * 0.96 +
                gas.argon * 0.86 +
                gas.carbonDioxide * 1.45 +
                gas.methane * 1.18 +
                gas.hydrogen * 0.20 +
                gas.helium * 0.07;

            return weighted / total;
        }

        double earthMolecularScatteringStrength()
        {
            AtmosphereComposition earth;
            earth.nitrogen = 0.78084;
            earth.oxygen = 0.20946;
            earth.argon = 0.00934;
            earth.carbonDioxide = 0.00042;
            earth.methane = 0.0000019;
            return molecularScatteringStrength(earth);
        }

        float approximateOceanLevel(float targetCoverage)
        {
            // The current five-octave FBM height field is centered near 0.5.
            // A local calibration of this exact terrain model puts ~71% water
            // around 0.548. This mapping is intentionally kept in the
            // derivation layer so a future geography model can replace it.
            const float coverage = std::clamp(targetCoverage, 0.0f, 1.0f);
            return std::clamp(
                0.50f + (coverage - 0.50f) * 0.30f,
                0.42f,
                0.62f);
        }

        float shaderSeed(std::uint32_t seed)
        {
            return static_cast<float>(seed % 10000u) * 0.017f;
        }

        ResolvedPlanet makeEarthLikeOceanWorld(std::uint32_t seed)
        {
            ResolvedPlanet planet;
            planet.masterSeed = seed == 0u ? 1u : seed;

            // Independent deterministic streams. Adding a cloud parameter later
            // should not move every continent.
            planet.terrainSeed = mixSeed(planet.masterSeed, 0x54455252u);    // TERR
            planet.atmosphereSeed = mixSeed(planet.masterSeed, 0x41544d4fu); // ATMO
            planet.cloudSeed = mixSeed(planet.masterSeed, 0x434c4f55u);      // CLOU
            planet.oceanSeed = mixSeed(planet.masterSeed, 0x4f434541u);      // OCEA

            // Earth-like physical base. Mass and gravity are derived below.
            planet.physical.radiusMeters = 6'371'000.0;
            planet.physical.bulkDensityKgPerCubicMeter = 5514.0;
            planet.physical.rotationPeriodSeconds = 86164.0905;
            planet.physical.axialTiltRadians = 23.4393 * std::numbers::pi / 180.0;
            planet.physical.meanSurfaceTemperatureKelvin = 288.15;
            planet.physical.bondAlbedo = 0.30;

            const double volume =
                (4.0 / 3.0) *
                std::numbers::pi *
                std::pow(planet.physical.radiusMeters, 3.0);

            planet.physical.massKg =
                planet.physical.bulkDensityKgPerCubicMeter * volume;

            planet.physical.surfaceGravityMetersPerSecondSquared =
                GravitationalConstant *
                planet.physical.massKg /
                std::pow(planet.physical.radiusMeters, 2.0);

            // Gas mixture and aerosols are separate inputs. This is intentionally
            // close to modern Earth, but the architecture no longer says
            // "breathable atmosphere => blue sky".
            planet.atmosphere.surfacePressurePascals = EarthPressurePascals;
            planet.atmosphere.composition.nitrogen = 0.78084;
            planet.atmosphere.composition.oxygen = 0.20946;
            planet.atmosphere.composition.argon = 0.00934;
            planet.atmosphere.composition.carbonDioxide = 0.00042;
            planet.atmosphere.composition.methane = 0.0000019;
            planet.atmosphere.aerosols.relativeDensity = 1.0f;
            planet.atmosphere.aerosols.scaleHeightKm = 1.2f;
            planet.atmosphere.aerosols.anisotropy = 0.8f;
            planet.atmosphere.ozoneRelativeStrength = 1.0f;
            planet.atmosphere.ozoneCenterHeightKm = 25.0f;
            planet.atmosphere.ozoneHalfWidthKm = 15.0f;

            planet.clouds.coverage = 0.58f;
            planet.clouds.baseAltitudeKm = 1.5f;
            planet.clouds.thicknessKm = 3.5f;
            planet.clouds.opticalThickness = 1.0f;

            // Surface values are procedural shaping controls. They are not
            // falsely derived from bulk chemistry. The water target drives sea
            // level; terrain structure remains replaceable later.
            planet.surface.hasLiquidOcean = true;
            planet.surface.targetOceanCoverage = 0.71f;
            planet.surface.seaLevelFieldValue =
                approximateOceanLevel(planet.surface.targetOceanCoverage);
            planet.surface.continentScale = 2.4f;
            planet.surface.detailScale = 10.0f;
            planet.surface.coastWidth = 0.018f;
            planet.surface.terrainReliefScale = 1.0f;

            planet.appearance.deepOceanColor = {0.006f, 0.022f, 0.055f};
            planet.appearance.shallowOceanColor = {0.020f, 0.100f, 0.145f};
            planet.appearance.lowLandColor = {0.075f, 0.19f, 0.050f};
            planet.appearance.highLandColor = {0.38f, 0.30f, 0.17f};
            planet.appearance.oceanRoughness = 0.10f;
            planet.appearance.landRoughness = 0.82f;
            planet.appearance.oceanWaveScale = 850.0f;
            planet.appearance.oceanWaveStrength = 0.18f;
            planet.appearance.oceanWaveSpeed = 0.65f;
            planet.appearance.atmosphereGroundAlbedo = {0.10f, 0.14f, 0.18f};

            return planet;
        }
    }

    ResolvedPlanet generatePlanet(const PlanetGenerationRequest& request)
    {
        switch (request.preset)
        {
        case PlanetPreset::EarthLikeOceanWorld:
        default:
            return makeEarthLikeOceanWorld(request.seed);
        }
    }

    AtmosphereParameters deriveAtmosphereParameters(const ResolvedPlanet& planet)
    {
        AtmosphereParameters parameters = makeEarthLikeAtmosphere();

        parameters.bottomRadiusKm =
            static_cast<float>(planet.physical.radiusMeters / 1000.0);

        if (planet.atmosphere.surfacePressurePascals <= 0.0)
        {
            parameters.topRadiusKm = parameters.bottomRadiusKm;
            parameters.rayleighScatteringPerKm = glm::vec3(0.0f);
            parameters.mieScatteringPerKm = glm::vec3(0.0f);
            parameters.mieExtinctionPerKm = glm::vec3(0.0f);
            parameters.ozoneAbsorptionPerKm = glm::vec3(0.0f);
            return parameters;
        }

        const double molarMass =
            meanMolarMassKgPerMol(planet.atmosphere.composition);

        const double gravity = std::max(
            planet.physical.surfaceGravityMetersPerSecondSquared,
            0.01);

        const double temperature = std::max(
            planet.physical.meanSurfaceTemperatureKelvin,
            1.0);

        const double scaleHeightMeters =
            UniversalGasConstant * temperature /
            (molarMass * gravity);

        parameters.rayleighScaleHeightKm =
            static_cast<float>(scaleHeightMeters / 1000.0);

        // Around a dozen scale heights captures essentially all molecular
        // density while still giving the renderer a finite atmosphere shell.
        const float atmosphereThicknessKm = std::clamp(
            parameters.rayleighScaleHeightKm * 12.0f,
            60.0f,
            300.0f);

        parameters.topRadiusKm =
            parameters.bottomRadiusKm + atmosphereThicknessKm;

        const double numberDensityRatio =
            (planet.atmosphere.surfacePressurePascals / temperature) /
            (EarthPressurePascals / EarthTemperatureKelvin);

        const double compositionRatio =
            molecularScatteringStrength(planet.atmosphere.composition) /
            earthMolecularScatteringStrength();

        parameters.rayleighScatteringPerKm *=
            static_cast<float>(numberDensityRatio * compositionRatio);

        const float aerosolScale = std::max(
            planet.atmosphere.aerosols.relativeDensity,
            0.0f);

        parameters.mieScatteringPerKm *= aerosolScale;
        parameters.mieExtinctionPerKm *= aerosolScale;
        parameters.mieScaleHeightKm = std::max(
            planet.atmosphere.aerosols.scaleHeightKm,
            0.01f);
        parameters.mieAnisotropy = std::clamp(
            planet.atmosphere.aerosols.anisotropy,
            -0.95f,
            0.95f);

        parameters.ozoneAbsorptionPerKm *= std::max(
            planet.atmosphere.ozoneRelativeStrength,
            0.0f);
        parameters.ozoneCenterHeightKm =
            planet.atmosphere.ozoneCenterHeightKm;
        parameters.ozoneHalfWidthKm = std::max(
            planet.atmosphere.ozoneHalfWidthKm,
            0.01f);

        parameters.groundAlbedo =
            planet.appearance.atmosphereGroundAlbedo;

        return parameters;
    }

    PlanetMaterial derivePlanetMaterial(const ResolvedPlanet& planet)
    {
        PlanetMaterial material;

        material.deepOceanColor = planet.appearance.deepOceanColor;
        material.shallowOceanColor = planet.appearance.shallowOceanColor;
        material.oceanRoughness = planet.appearance.oceanRoughness;
        material.oceanWaveScale = planet.appearance.oceanWaveScale;
        material.oceanWaveStrength = planet.appearance.oceanWaveStrength;
        material.oceanWaveSpeed = planet.appearance.oceanWaveSpeed;

        material.lowLandColor = planet.appearance.lowLandColor;
        material.highLandColor = planet.appearance.highLandColor;
        material.landRoughness = planet.appearance.landRoughness;

        material.continentScale = planet.surface.continentScale;
        material.detailScale = planet.surface.detailScale;
        material.oceanLevel = planet.surface.hasLiquidOcean
            ? planet.surface.seaLevelFieldValue
            : -2.0f;
        material.coastWidth = planet.surface.coastWidth;
        material.terrainReliefScale = planet.surface.terrainReliefScale;
        material.seed = shaderSeed(planet.terrainSeed);

        return material;
    }
}
