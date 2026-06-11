#include "world/PlanetVisualGenerator.h"

namespace SpaceSim
{
    namespace
    {
        static Color MakeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
        {
            return Color{ r, g, b, a };
        }

        static void ApplyAtmosphere(PlanetVisual& visual, AtmosphereType atmosphere)
        {
            switch (atmosphere)
            {
            case AtmosphereType::None:
                visual.atmosphereColor = BLANK;
                visual.atmosphereStrength = 0.0f;
                visual.hasClouds = false;
                break;

            case AtmosphereType::Thin:
                visual.atmosphereColor = MakeColor(120, 160, 255, 70);
                visual.atmosphereStrength = 0.25f;
                visual.hasClouds = false;
                break;

            case AtmosphereType::Breathable:
                visual.atmosphereColor = MakeColor(100, 160, 255, 110);
                visual.atmosphereStrength = 0.45f;
                visual.hasClouds = true;
                break;

            case AtmosphereType::Thick:
                visual.atmosphereColor = MakeColor(210, 180, 120, 150);
                visual.atmosphereStrength = 0.7f;
                visual.hasClouds = true;
                break;

            case AtmosphereType::Toxic:
                visual.atmosphereColor = MakeColor(160, 220, 90, 140);
                visual.atmosphereStrength = 0.65f;
                visual.hasClouds = true;
                break;

            case AtmosphereType::HydrogenHelium:
                visual.atmosphereColor = MakeColor(230, 190, 130, 170);
                visual.atmosphereStrength = 0.85f;
                visual.hasClouds = true;
                break;
            }
        }

        static PlanetVisual GenerateBaseVisual(const PlanetData& data)
        {
            PlanetVisual visual{};

            switch (data.planetClass)
            {
            case PlanetClass::Rocky:
                visual.baseColor = MakeColor(130, 105, 85);
                visual.secondaryColor = MakeColor(80, 70, 65);
                visual.surfaceNoiseStrength = 0.65f;
                break;

            case PlanetClass::BarrenMoon:
                visual.baseColor = MakeColor(125, 125, 125);
                visual.secondaryColor = MakeColor(75, 75, 75);
                visual.surfaceNoiseStrength = 0.8f;
                break;

            case PlanetClass::IceWorld:
                visual.baseColor = MakeColor(190, 220, 235);
                visual.secondaryColor = MakeColor(90, 145, 190);
                visual.surfaceNoiseStrength = 0.45f;
                break;

            case PlanetClass::DesertWorld:
                visual.baseColor = MakeColor(195, 145, 75);
                visual.secondaryColor = MakeColor(120, 80, 45);
                visual.surfaceNoiseStrength = 0.55f;
                break;

            case PlanetClass::OceanWorld:
                visual.baseColor = MakeColor(45, 90, 170);
                visual.secondaryColor = MakeColor(60, 160, 120);
                visual.surfaceNoiseStrength = 0.35f;
                break;

            case PlanetClass::LavaWorld:
                visual.baseColor = MakeColor(45, 35, 30);
                visual.secondaryColor = MakeColor(255, 90, 20);
                visual.surfaceNoiseStrength = 0.9f;
                break;

            case PlanetClass::GasGiant:
                visual.baseColor = MakeColor(210, 160, 95);
                visual.secondaryColor = MakeColor(120, 75, 45);
                visual.bandStrength = 1.0f;
                visual.surfaceNoiseStrength = 0.2f;
                break;

            case PlanetClass::IceGiant:
                visual.baseColor = MakeColor(90, 170, 210);
                visual.secondaryColor = MakeColor(160, 220, 230);
                visual.bandStrength = 0.55f;
                visual.surfaceNoiseStrength = 0.2f;
                break;

            case PlanetClass::CarbonWorld:
                visual.baseColor = MakeColor(24, 22, 24);
                visual.secondaryColor = MakeColor(95, 42, 32);
                visual.surfaceNoiseStrength = 0.75f;
                break;

            default:
                visual.baseColor = WHITE;
                visual.secondaryColor = GRAY;
                break;
            }

            ApplyAtmosphere(visual, data.atmosphere);

            if (data.visualOverrides.baseColor.has_value())
            {
                visual.baseColor = data.visualOverrides.baseColor.value();
            }

            if (data.visualOverrides.secondaryColor.has_value())
            {
                visual.secondaryColor = data.visualOverrides.secondaryColor.value();
            }

            if (data.visualOverrides.atmosphereColor.has_value())
            {
                visual.atmosphereColor = data.visualOverrides.atmosphereColor.value();
            }

            return visual;
        }
    }

    PlanetVisual GeneratePlanetVisual(const GlobalObject& object)
    {
        if (!object.hasPlanetData)
        {
            PlanetVisual visual{};
            visual.baseColor = object.color;
            visual.secondaryColor = object.color;
            visual.atmosphereColor = BLANK;
            visual.atmosphereStrength = 0.0f;
            return visual;
        }

        return GenerateBaseVisual(object.planetData);
    }
}