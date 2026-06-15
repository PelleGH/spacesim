#include "PlanetRenderer.h"

#include <raymath.h>
#include <rlgl.h>
#include <string>

namespace SpaceSim
{
    static void DrawModelScaled(Model& model, const Shader& shader, Vector3 position, float radius)
    {
        model.materials[0].shader = shader;

        DrawModelEx(
            model,
            position,
            Vector3{ 0.0f, 1.0f, 0.0f },
            0.0f,
            Vector3{ radius, radius, radius },
            WHITE
        );
    }

    PlanetRenderer::PlanetRenderer() = default;

    PlanetRenderer::~PlanetRenderer()
    {
        shutdown();
    }

    void PlanetRenderer::initialize()
    {
        if (m_initialized)
        {
            return;
        }

        Mesh sphereMesh = GenMeshSphere(1.0f, 192, 96);
        m_sphereModel = LoadModelFromMesh(sphereMesh);
TraceLog(LOG_WARNING, "Working directory: %s", GetWorkingDirectory());

TraceLog(
    LOG_WARNING,
    "Surface VS exists: %s",
    FileExists("data/shaders/planet_surface.vs") ? "YES" : "NO"
);

TraceLog(
    LOG_WARNING,
    "Surface FS exists: %s",
    FileExists("data/shaders/planet_surface.fs") ? "YES" : "NO"
);
        m_surfaceShader = LoadShader(
            "data/shaders/planet_surface.vs",
            "data/shaders/planet_surface.fs"
        );

        m_cloudShader = LoadShader(
            "data/shaders/planet_surface.vs",
            "data/shaders/planet_clouds.fs"
        );

        m_atmosphereShader = LoadShader(
            "data/shaders/planet_surface.vs",
            "data/shaders/planet_atmosphere.fs"
        );
        m_gasGiantShader = LoadShader(
            "data/shaders/planet_surface.vs",
            "data/shaders/planet_gas_giant.fs"
        );
        if (m_surfaceShader.id == 0)
        {
            TraceLog(LOG_ERROR, "Failed to load planet surface shader");
        }

        if (m_cloudShader.id == 0)
        {
            TraceLog(LOG_ERROR, "Failed to load planet cloud shader");
        }

        if (m_atmosphereShader.id == 0)
        {
            TraceLog(LOG_ERROR, "Failed to load planet atmosphere shader");
        }
        if (m_gasGiantShader.id == 0)
        {
            TraceLog(LOG_ERROR, "Failed to load gas giant shader");
        }
        m_surfaceLightDirLoc = GetShaderLocation(m_surfaceShader, "lightDir");
        m_surfaceCameraPosLoc = GetShaderLocation(m_surfaceShader, "cameraPos");
        m_surfaceSeedLoc = GetShaderLocation(m_surfaceShader, "seed");
        m_surfaceSeaLevelLoc = GetShaderLocation(m_surfaceShader, "seaLevel");
        m_surfacePlanetTypeLoc = GetShaderLocation(m_surfaceShader, "planetType");
        m_cloudLightDirLoc = GetShaderLocation(m_cloudShader, "lightDir");
        m_cloudCameraPosLoc = GetShaderLocation(m_cloudShader, "cameraPos");
        m_cloudSeedLoc = GetShaderLocation(m_cloudShader, "seed");
        m_cloudStrengthLoc = GetShaderLocation(m_cloudShader, "cloudStrength");

        m_atmoLightDirLoc = GetShaderLocation(m_atmosphereShader, "lightDir");
        m_atmoCameraPosLoc = GetShaderLocation(m_atmosphereShader, "cameraPos");
        m_atmoStrengthLoc = GetShaderLocation(m_atmosphereShader, "atmosphereStrength");

        m_gasLightDirLoc = GetShaderLocation(m_gasGiantShader, "lightDir");
        m_gasCameraPosLoc = GetShaderLocation(m_gasGiantShader, "cameraPos");
        m_gasSeedLoc = GetShaderLocation(m_gasGiantShader, "seed");
        m_gasColorALoc = GetShaderLocation(m_gasGiantShader, "colorA");
        m_gasColorBLoc = GetShaderLocation(m_gasGiantShader, "colorB");
        m_gasColorCLoc = GetShaderLocation(m_gasGiantShader, "colorC");

        m_initialized = true;
    }

    void PlanetRenderer::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        UnloadShader(m_surfaceShader);
        UnloadShader(m_cloudShader);
        UnloadShader(m_atmosphereShader);
        UnloadShader(m_gasGiantShader);
        UnloadModel(m_sphereModel);

        m_surfaceShader = {};
        m_cloudShader = {};
        m_atmosphereShader = {};
        m_gasGiantShader = {};
        m_sphereModel = {};

        m_initialized = false;
    }

    void PlanetRenderer::drawSun(Vector3 position, float radius, Color color)
    {
        // The sun does not need the planet mesh or planet shaders.
        DrawSphere(position, radius, color);

        BeginBlendMode(BLEND_ADDITIVE);
        DrawSphere(position, radius * 1.08f, Color{ color.r, color.g, color.b, 80 });
        DrawSphere(position, radius * 1.20f, Color{ color.r, color.g, color.b, 35 });
        EndBlendMode();
    }

    void PlanetRenderer::drawEarthLikePlanet(const Camera3D& camera, const PlanetRenderParams& params)
    {
        if (!m_initialized)
        {
            initialize();
        }

        Vector3 lightDir = Vector3Normalize(params.lightDirection);
        Vector3 cameraPos = camera.position;

        int safeSeedInt = params.seed % 10000;
        if (safeSeedInt < 0)
        {
            safeSeedInt += 10000;
        }

        float seed = static_cast<float>(safeSeedInt) / 10000.0f;
        float seaLevel = params.seaLevel;
        float cloudStrength = params.cloudStrength;
        float atmosphereStrength = params.atmosphereStrength;
        int planetType = params.planetType;

        // Surface pass.
        BeginShaderMode(m_surfaceShader);
        SetShaderValue(m_surfaceShader, m_surfaceLightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(m_surfaceShader, m_surfaceCameraPosLoc, &cameraPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(m_surfaceShader, m_surfaceSeedLoc, &seed, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_surfaceShader, m_surfaceSeaLevelLoc, &seaLevel, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_surfaceShader, m_surfacePlanetTypeLoc, &planetType, SHADER_UNIFORM_INT);
        DrawModelScaled(m_sphereModel, m_surfaceShader, params.position, params.radius);
        EndShaderMode();

        if (cloudStrength > 0.001f)
        {
            BeginBlendMode(BLEND_ALPHA);
            BeginShaderMode(m_cloudShader);
            SetShaderValue(m_cloudShader, m_cloudLightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
            SetShaderValue(m_cloudShader, m_cloudCameraPosLoc, &cameraPos, SHADER_UNIFORM_VEC3);
            SetShaderValue(m_cloudShader, m_cloudSeedLoc, &seed, SHADER_UNIFORM_FLOAT);
            SetShaderValue(m_cloudShader, m_cloudStrengthLoc, &cloudStrength, SHADER_UNIFORM_FLOAT);
            DrawModelScaled(m_sphereModel, m_cloudShader, params.position, params.radius * 1.006f);
            EndShaderMode();
            EndBlendMode();
        }

        if (atmosphereStrength > 0.001f)
        {
            BeginBlendMode(BLEND_ALPHA);
            BeginShaderMode(m_atmosphereShader);
            SetShaderValue(m_atmosphereShader, m_atmoLightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
            SetShaderValue(m_atmosphereShader, m_atmoCameraPosLoc, &cameraPos, SHADER_UNIFORM_VEC3);
            SetShaderValue(m_atmosphereShader, m_atmoStrengthLoc, &atmosphereStrength, SHADER_UNIFORM_FLOAT);
            DrawModelScaled(m_sphereModel, m_atmosphereShader, params.position, params.radius * 1.018f);
            EndShaderMode();
            EndBlendMode();
        }
    }
    void PlanetRenderer::drawGasGiantPlanet(const Camera3D& camera, const PlanetRenderParams& params)
    {
        if (!m_initialized)
        {
            initialize();
        }

        Vector3 lightDir = Vector3Normalize(params.lightDirection);
        Vector3 cameraPos = camera.position;

        int safeSeedInt = params.seed % 10000;
        if (safeSeedInt < 0)
        {
            safeSeedInt += 10000;
        }

        float seed = static_cast<float>(safeSeedInt) / 10000.0f;

        Vector3 colorA{ 0.56f, 0.39f, 0.22f }; // tan belts
        Vector3 colorB{ 0.96f, 0.84f, 0.62f }; // cream zones
        Vector3 colorC{ 0.22f, 0.13f, 0.075f }; // dark brown belts

        BeginShaderMode(m_gasGiantShader);
        SetShaderValue(m_gasGiantShader, m_gasLightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(m_gasGiantShader, m_gasCameraPosLoc, &cameraPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(m_gasGiantShader, m_gasSeedLoc, &seed, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_gasGiantShader, m_gasColorALoc, &colorA, SHADER_UNIFORM_VEC3);
        SetShaderValue(m_gasGiantShader, m_gasColorBLoc, &colorB, SHADER_UNIFORM_VEC3);
        SetShaderValue(m_gasGiantShader, m_gasColorCLoc, &colorC, SHADER_UNIFORM_VEC3);
        DrawModelScaled(m_sphereModel, m_gasGiantShader, params.position, params.radius);
        EndShaderMode();

        if (params.atmosphereStrength > 0.001f)
        {
            float atmosphereStrength = params.atmosphereStrength * 0.6f;

            BeginBlendMode(BLEND_ALPHA);
            BeginShaderMode(m_atmosphereShader);
            SetShaderValue(m_atmosphereShader, m_atmoLightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
            SetShaderValue(m_atmosphereShader, m_atmoCameraPosLoc, &cameraPos, SHADER_UNIFORM_VEC3);
            SetShaderValue(m_atmosphereShader, m_atmoStrengthLoc, &atmosphereStrength, SHADER_UNIFORM_FLOAT);
            DrawModelScaled(m_sphereModel, m_atmosphereShader, params.position, params.radius * 1.012f);
            EndShaderMode();
            EndBlendMode();
        }
    }
}
