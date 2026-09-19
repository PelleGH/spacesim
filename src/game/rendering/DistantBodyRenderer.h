#pragma once

#include "world/GameWorld.h"

#include <raylib.h>

namespace SpaceSim
{
    class DistantBodyRenderer
    {
    public:
        void render(const GameWorld& world);

        static Shader oceanSurfaceShader(const GlobalObject& object,
            const SceneLighting& lighting,
            Vector3 center, float radius, Vector3 cameraPosition,
            int lightingDebugMode = 2);

        void renderLocalPlanet(
            const GlobalObject& object,
            const SceneLighting& lighting,
            Vector3 position,
            float radius,
            float atmosphereMultiplier = 1.0f,
            Vector3 cameraPosition = {},
            bool drawSurface = true,
            bool drawLayers = true);
    };
}
