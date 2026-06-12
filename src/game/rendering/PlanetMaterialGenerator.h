#pragma once

#include "world/StarSystem.h"

#include <raylib.h>

namespace SpaceSim
{
    struct PlanetMaterialMaps
    {
        Texture2D albedo{};    // RGB: raw surface colour, A: solid/land mask for shader blending
        Texture2D normal{};    // RGB: object-space normal encoded 0..1
        Texture2D material{};  // R: water/smooth mask, G: coast/secondary/emissive hint, B: roughness, A: relief
    };

    PlanetMaterialMaps GeneratePlanetMaterialMaps(
        const GlobalObject& object,
        int width = 1024,
        int height = 512
    );
}
