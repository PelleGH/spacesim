#pragma once

#include "world/StarSystem.h"

#include <raylib.h>

namespace SpaceSim
{
    struct PlanetRenderParams
    {
        Vector3 position{};
        float radius = 1.0f;

        Vector3 lightDirection{};
        int seed = 1;

        float seaLevel = 0.50f;
        float atmosphereStrength = 1.0f;
        float cloudStrength = 1.0f;

        int planetType = 0;
    };

    class PlanetRenderer
    {
    public:
        PlanetRenderer();
        ~PlanetRenderer();

        PlanetRenderer(const PlanetRenderer&) = delete;
        PlanetRenderer& operator=(const PlanetRenderer&) = delete;

        void initialize();
        void shutdown();

        void drawEarthLikePlanet(const Camera3D& camera, const PlanetRenderParams& params);
        void drawGasGiantPlanet(const Camera3D& camera, const PlanetRenderParams& params);
        void drawSun(Vector3 position, float radius, Color color);

        

    private:
        bool m_initialized = false;

        Model m_sphereModel{};

        Shader m_surfaceShader{};
        Shader m_cloudShader{};
        Shader m_atmosphereShader{};
        Shader m_gasGiantShader{};

        int m_gasLightDirLoc = -1;
        int m_gasCameraPosLoc = -1;
        int m_gasSeedLoc = -1;
        int m_gasColorALoc = -1;
        int m_gasColorBLoc = -1;
        int m_gasColorCLoc = -1;
        
        int m_surfacePlanetTypeLoc = -1;
        int m_surfaceLightDirLoc = -1;
        int m_surfaceCameraPosLoc = -1;
        int m_surfaceSeedLoc = -1;
        int m_surfaceSeaLevelLoc = -1;

        int m_cloudLightDirLoc = -1;
        int m_cloudCameraPosLoc = -1;
        int m_cloudSeedLoc = -1;
        int m_cloudStrengthLoc = -1;

        int m_atmoLightDirLoc = -1;
        int m_atmoCameraPosLoc = -1;
        int m_atmoStrengthLoc = -1;
    };
}