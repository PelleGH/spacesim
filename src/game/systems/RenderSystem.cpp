#include "systems/RenderSystem.h"

#include "components/RenderableComponent.h"
#include "components/TransformComponent.h"

#include <algorithm>
#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

namespace SpaceSim
{
    OrbitalPlanetRenderInfo RenderSystem::getActiveOrbitalPlanetRenderInfo(
        const GameWorld& world) const
    {
        OrbitalPlanetRenderInfo info{};

        const PlanetTransitionState& transition = world.planetTransition;

        if (transition.mode == PlanetRenderMode::Distant)
        {
            return info;
        }

        if (transition.closestPlanetIndex < 0 ||
            transition.closestPlanetIndex >= static_cast<int>(world.starSystem.objects.size()))
        {
            return info;
        }

        const GlobalObject& planet = world.starSystem.objects[transition.closestPlanetIndex];

        if (planet.type != GlobalObjectType::Planet)
        {
            return info;
        }

        const DVec3 relativeGlobal = planet.position - world.globalPlayerPosition;
        const double centerDistance = Length(relativeGlobal);

        if (centerDistance <= 0.000001)
        {
            return info;
        }

        const DVec3 directionGlobal = Normalize(relativeGlobal);
        const Vector3 direction{
            static_cast<float>(directionGlobal.x),
            static_cast<float>(directionGlobal.y),
            static_cast<float>(directionGlobal.z)
        };

        constexpr float orbitalRenderDistance = 700.0f;

        const double angularRadius = std::atan2(planet.visualRadius, centerDistance);

        info.radius = std::max(
            1.0f,
            static_cast<float>(std::tan(angularRadius) * orbitalRenderDistance));

        info.position = Vector3Add(
            world.camera.position,
            Vector3Scale(direction, orbitalRenderDistance));

        info.valid = true;
        return info;
    }

    void RenderSystem::drawActiveOrbitalPlanet(GameWorld& world, bool drawSurface)
    {
        const PlanetTransitionState& transition = world.planetTransition;

        if (transition.mode == PlanetRenderMode::Distant)
        {
            return;
        }

        if (transition.closestPlanetIndex < 0 ||
            transition.closestPlanetIndex >= static_cast<int>(world.starSystem.objects.size()))
        {
            return;
        }

        const GlobalObject& planet = world.starSystem.objects[transition.closestPlanetIndex];
        const OrbitalPlanetRenderInfo planetInfo = getActiveOrbitalPlanetRenderInfo(world);

        if (!planetInfo.valid)
        {
            return;
        }

        float atmosphereMultiplier = 1.0f;
        if (world.atmosphere.insideAtmosphere)
        {
            atmosphereMultiplier = 1.5f + world.atmosphere.density * 1.5f;
        }

        m_distantBodyRenderer.renderLocalPlanet(
            planet,
            world.lighting,
            planetInfo.position,
            planetInfo.radius,
            atmosphereMultiplier,
            world.camera.position, drawSurface, m_showPlanetLayers);
    }

    void RenderSystem::renderSky(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        Vector3 cameraForward = Vector3Normalize(Vector3Subtract(
            world.camera.target,
            world.camera.position
        ));

        Camera3D skyCamera{};
        skyCamera.position = Vector3{ 0.0f, 0.0f, 0.0f };
        skyCamera.target = cameraForward;
        skyCamera.up = world.camera.up;
        skyCamera.fovy = world.camera.fovy;
        skyCamera.projection = world.camera.projection;

        BeginMode3D(skyCamera);

        rlDisableDepthTest();
        rlDisableDepthMask();

        m_spaceBackgroundRenderer.render(skyCamera);
        rlDrawRenderBatchActive();
        rlSetTexture(0);

        rlEnableDepthTest();
        rlEnableDepthMask();

        m_distantBodyRenderer.render(world);
        rlDrawRenderBatchActive();

        rlEnableDepthMask();
        rlEnableDepthTest();

        EndMode3D();
    }

    void RenderSystem::renderWorld(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        if (world.travelMode == TravelMode::FTLTravel)
        {
            return;
        }

        const OrbitalPlanetRenderInfo planetInfo = getActiveOrbitalPlanetRenderInfo(world);
        if (IsKeyPressed(KEY_F6)) m_useAdaptivePlanet = !m_useAdaptivePlanet;
        if (IsKeyPressed(KEY_F7)) m_showPlanetLayers = !m_showPlanetLayers;
        if (IsKeyPressed(KEY_F8)) m_terrainLightingMode = (m_terrainLightingMode + 1) % 3;
        bool adaptiveReady = false;
        if (m_useAdaptivePlanet && planetInfo.valid) {
            const auto& planet = world.starSystem.objects[world.planetTransition.closestPlanetIndex];
            adaptiveReady = m_adaptivePlanet.update(planet, planetInfo.position, planetInfo.radius,
                world.camera, GetScreenHeight());
            if (adaptiveReady) m_adaptivePlanet.draw(planet, world.lighting, planetInfo.position,
                planetInfo.radius, world.camera.position, m_terrainLightingMode);
        }
        if (!adaptiveReady) {
            m_surfacePatch.update(world, planetInfo, m_terrainLightingMode);
            m_surfacePatch.draw();
        }
        // Once roots are resident the adaptive tree replaces the entire local globe.
        drawActiveOrbitalPlanet(world, !adaptiveReady);

        auto view = world.registry.view<TransformComponent, RenderableComponent>();

        for (auto entity : view)
        {
            const auto& transform = view.get<TransformComponent>(entity);
            const auto& renderable = view.get<RenderableComponent>(entity);

            m_prototypeMeshRenderer.render(transform, renderable);
        }
    }

    void RenderSystem::renderAtmosphereOverlay(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        if (world.planetTransition.mode != PlanetRenderMode::Distant) {
            const char* lightingMode = m_terrainLightingMode == 0 ? "Albedo" :
                (m_terrainLightingMode == 1 ? "Normals" : "Lit");
            DrawText(TextFormat("Planet: %s [F6]   Layers: %s [F7]   Terrain: %s [F8]",
                m_useAdaptivePlanet ? "Adaptive" : "Original",
                m_showPlanetLayers ? "On" : "Off", lightingMode),
                12, GetScreenHeight()-26, 16, LIGHTGRAY);
        }
        if (!world.atmosphere.insideAtmosphere || !m_showPlanetLayers)
        {
            return;
        }

        const float density = Clamp(world.atmosphere.density, 0.0f, 1.0f);
        const float hazeStrength = density * density;

        // Most aerial perspective is handled by the planet material. Keep this
        // full-screen tint subtle so it does not wash out nearby terrain.
        const unsigned char alpha = static_cast<unsigned char>(hazeStrength * 8.0f);

        DrawRectangle(
            0,
            0,
            GetScreenWidth(),
            GetScreenHeight(),
            Color{ 80, 145, 205, alpha });
    }
}
