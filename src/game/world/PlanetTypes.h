#pragma once

#include <cstdint>
#include <optional>
#include <raylib.h>

namespace SpaceSim
{
    enum class PlanetClass
    {
        None,
        Rocky,
        BarrenMoon,
        IceWorld,
        DesertWorld,
        OceanWorld,
        LavaWorld,
        GasGiant,
        IceGiant,
        CarbonWorld
    };

    enum class PlanetComposition
    {
        Unknown,
        SilicateIron,
        IceRock,
        CarbonRich,
        Metallic,
        HydrogenHelium,
        MethaneAmmonia,
        Sulfuric
    };

    enum class AtmosphereType
    {
        None,
        Thin,
        Breathable,
        Thick,
        Toxic,
        HydrogenHelium
    };

    struct PlanetVisualOverrides
    {
        std::optional<Color> baseColor;
        std::optional<Color> secondaryColor;
        std::optional<Color> atmosphereColor;
    };

    struct PlanetData
    {
        PlanetClass planetClass = PlanetClass::Rocky;
        PlanetComposition composition = PlanetComposition::SilicateIron;
        AtmosphereType atmosphere = AtmosphereType::None;

        double temperatureK = 280.0;
        std::uint32_t seed = 1;

        PlanetVisualOverrides visualOverrides{};
    };

    struct PlanetVisual
    {
        Color baseColor = WHITE;
        Color secondaryColor = GRAY;
        Color atmosphereColor = BLANK;

        float atmosphereStrength = 0.0f;
        float bandStrength = 0.0f;
        float surfaceNoiseStrength = 0.5f;
        bool hasClouds = false;
    };
}