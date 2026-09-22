#pragma once

#include "game/planet/PlanetGeneration.h"
#include "renderer/atmosphere/AtmosphereParameters.h"
#include "renderer/planet/PlanetMaterial.h"

namespace SpaceSim
{
    // Canonical resolved planet state. Gameplay and future simulation systems
    // should read physical/environmental truth from here rather than infer it
    // from rendering presets.
    struct PlanetComponent
    {
        ResolvedPlanet properties{};
    };

    // Renderer-facing surface values derived from PlanetComponent::properties.
    // Keeping the derivation explicit means the renderer never needs to know
    // which generation preset created the planet.
    struct PlanetVisualComponent
    {
        PlanetMaterial material;
    };

    // Renderer-facing atmospheric values derived from the resolved atmosphere
    // definition. Gas composition/aerosols remain stored in PlanetComponent.
    struct AtmosphereComponent
    {
        AtmosphereParameters parameters = makeEarthLikeAtmosphere();
        bool enabled = true;
    };
}
