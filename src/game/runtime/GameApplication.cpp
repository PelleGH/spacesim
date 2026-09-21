#include "game/runtime/GameApplication.h"

#include "game/bootstrap/PrototypeWorld.h"
#include "game/ecs/components/GlobalPositionComponent.h"
#include "game/ecs/components/HyperdriveComponents.h"
#include "game/ecs/components/PlanetComponents.h"
#include "game/ecs/components/ShipControlComponent.h"
#include "game/ecs/components/ShipMovementComponent.h"
#include "game/ecs/components/TransformComponent.h"
#include "game/rendering/GameRenderResources.h"
#include "game/world/SpaceScale.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <glm/geometric.hpp>

namespace SpaceSim
{
    GameApplication::GameApplication()
        : m_window(1280, 720, "SpaceSim")
    {
        createPrototypeWorld(m_world);

        if (m_world.primaryPlanet == entt::null ||
            !m_world.registry.all_of<AtmosphereComponent>(m_world.primaryPlanet))
        {
            throw std::runtime_error("Prototype world did not create an atmosphere.");
        }

        const auto& atmosphere = m_world.registry.get<AtmosphereComponent>(m_world.primaryPlanet);
        m_renderResources = std::make_unique<GameRenderResources>(atmosphere.parameters);

        m_renderer.setExposure(1.0f);
        m_renderer.setBloomStrength(0.045f);
        m_renderer.setBloomThreshold(8.0f);
        m_renderer.setAtmosphereLightingEnabled(true);
        m_renderer.setAtmosphereSpecularEnabled(true);

        // Place the chase camera correctly before the first rendered frame.
        m_chaseCameraSystem.update(m_world, 0.0f);

        std::cout
            << "\nSpaceSim flight toy\n"
            << "  ECS: EnTT\n"
            << "  Platform: SDL3\n"
            << "  Renderer: OpenGL renderer pipeline\n"
            << "  Fixed simulation: 120 Hz\n"
            << "  Local gameplay units: metres / seconds\n"
            << "  Placeholder ship: ~28 m long\n"
            << "  Planet distance: 6 planetary radii (global layer)\n"
            << "  Planet render: distant angular-size -> physical near-body handoff\n"
            << "  Near-body transition: 1500 km altitude\n"
            << "  Local bubble rebase: 10 km from local origin\n\n"
            << "Flight controls:\n"
            << "  W / S       = forward / reverse thrust\n"
            << "  A / D       = strafe left / right\n"
            << "  Space / Ctrl= thrust up / down\n"
            << "  Hold RMB + mouse = pitch / yaw\n"
            << "  Arrow keys  = pitch / yaw (keyboard)\n"
            << "  Q / E       = roll left / right\n"
            << "  Shift       = boost\n"
            << "  F           = toggle flight assist\n"
            << "  Tab         = cycle hyperdrive jump points\n"
            << "  J           = align + hyperdrive toward selected point\n"
            << "  L           = atmospheric lighting\n"
            << "  R           = atmospheric reflections\n"
            << "  F11         = fullscreen\n"
            << "  Escape      = quit\n\n"
            << "Scale references:\n"
            << "  10 m cube  @ ~132 m\n"
            << "  25 m cube  @ ~351 m\n"
            << "  100 m cube @ ~919 m\n"
            << "  250 m cube @ ~2224 m\n\n";
    }

    GameApplication::~GameApplication() = default;

    int GameApplication::run()
    {
        while (m_window.processEvents())
        {
            const double frameDelta = std::min(m_clock.tick(), MaxFrameDeltaSeconds);
            m_accumulator += frameDelta;

            m_input.beginFrame();
            handleApplicationInput();
            sampleGameplayInput(static_cast<float>(frameDelta));

            while (m_accumulator >= FixedDeltaSeconds)
            {
                fixedUpdate(static_cast<float>(FixedDeltaSeconds));
                m_accumulator -= FixedDeltaSeconds;
            }

            update(static_cast<float>(frameDelta));

            if (m_window.pixelWidth() <= 0 || m_window.pixelHeight() <= 0)
            {
                SDL_Delay(10);
                continue;
            }

            render();
            m_window.swapBuffers();
        }

        return 0;
    }

    void GameApplication::handleApplicationInput()
    {
        if (m_input.keyPressed(SDL_SCANCODE_F11))
        {
            m_window.toggleFullscreen();
        }

        // Mouse steering uses relative mode while RMB is held. Keeping capture
        // here (rather than in the ship system) keeps platform/window behavior
        // out of gameplay systems.
        if (m_input.mousePressed(MouseButton::Right))
        {
            m_window.captureMouse(true);
        }
        else if (m_input.mouseReleased(MouseButton::Right))
        {
            m_window.captureMouse(false);
        }

        if (m_input.keyPressed(SDL_SCANCODE_L))
        {
            const bool enabled = !m_renderer.atmosphereLightingEnabled();
            m_renderer.setAtmosphereLightingEnabled(enabled);
            std::cout << "Atmospheric lighting: " << (enabled ? "ON" : "OFF") << '\n';
        }

        if (m_input.keyPressed(SDL_SCANCODE_R))
        {
            const bool enabled = !m_renderer.atmosphereSpecularEnabled();
            m_renderer.setAtmosphereSpecularEnabled(enabled);
            std::cout << "Atmospheric reflections: " << (enabled ? "ON" : "OFF") << '\n';
        }
    }

    void GameApplication::sampleGameplayInput(float frameDt)
    {
        // Input is sampled once per rendered frame and written into ECS control
        // intent. Fixed-rate movement consumes that intent below.
        m_playerControlSystem.update(m_world, m_input, frameDt);
        m_hyperdriveSystem.update(m_world);
    }

    void GameApplication::fixedUpdate(float dt)
    {
        // Hyperdrive owns the ship transform while aligning/traveling. Normal
        // local-flight movement automatically skips active hyperdrive ships.
        m_hyperdriveSystem.fixedUpdate(m_world, dt);
        m_shipMovementSystem.fixedUpdate(m_world, dt);
        m_localBubbleRebaseSystem.fixedUpdate(m_world);
    }

    void GameApplication::update(float dt)
    {
        // Presentation systems may run at render rate. The camera follows the
        // latest fixed-step ship transform without owning ship simulation.
        m_chaseCameraSystem.update(m_world, dt);
        updateDebugTitle(dt);
    }

    void GameApplication::updateDebugTitle(float dt)
    {
        m_titleUpdateAccumulator += static_cast<double>(dt);
        if (m_titleUpdateAccumulator < 0.20)
        {
            return;
        }
        m_titleUpdateAccumulator = 0.0;

        if (m_world.playerShip == entt::null ||
            !m_world.registry.valid(m_world.playerShip) ||
            !m_world.registry.all_of<ShipMovementComponent, ShipControlComponent>(m_world.playerShip))
        {
            return;
        }

        const auto& movement = m_world.registry.get<ShipMovementComponent>(m_world.playerShip);
        const auto& control = m_world.registry.get<ShipControlComponent>(m_world.playerShip);

        const double speedMetersPerSecond = glm::length(movement.velocityMetersPerSecond);
        const double speedKilometersPerHour = speedMetersPerSecond * 3.6;

        std::ostringstream title;
        title << "SpaceSim | ";

        if (m_world.registry.all_of<HyperdriveComponent>(m_world.playerShip))
        {
            const auto& hyperdrive =
                m_world.registry.get<HyperdriveComponent>(m_world.playerShip);

            if (hyperdrive.state == HyperdriveState::Aligning)
            {
                title << "Hyperdrive ALIGNING";
            }
            else if (hyperdrive.state == HyperdriveState::Traveling)
            {
                title << std::fixed << std::setprecision(0)
                      << "Hyperdrive "
                      << hyperdrive.travelSpeedMetersPerSecond / SpaceScale::MetersPerKilometer
                      << " km/s | "
                      << hyperdrive.remainingDistanceMeters / SpaceScale::MetersPerKilometer
                      << " km remaining";
            }
            else
            {
                title << std::fixed << std::setprecision(1)
                      << speedMetersPerSecond << " m/s | "
                      << speedKilometersPerHour << " km/h | Assist "
                      << (control.flightAssist ? "ON" : "OFF");
            }
        }

        if (m_world.primaryPlanet != entt::null &&
            m_world.registry.valid(m_world.primaryPlanet) &&
            m_world.registry.all_of<GlobalPositionComponent, PlanetComponent>(m_world.primaryPlanet) &&
            m_world.registry.all_of<TransformComponent>(m_world.playerShip))
        {
            const auto& planetPosition =
                m_world.registry.get<GlobalPositionComponent>(m_world.primaryPlanet);
            const auto& planet =
                m_world.registry.get<PlanetComponent>(m_world.primaryPlanet);
            const auto& shipTransform =
                m_world.registry.get<TransformComponent>(m_world.playerShip);

            const glm::dvec3 shipGlobalMeters =
                m_world.localToGlobalMeters(shipTransform.positionMeters);
            const double altitudeMeters =
                glm::length(shipGlobalMeters - planetPosition.positionMeters) -
                planet.radiusMeters;

            const bool nearBody =
                altitudeMeters <= SpaceScale::NearBodyTransitionAltitudeMeters;

            title << std::fixed << std::setprecision(1)
                  << " | Alt "
                  << altitudeMeters / SpaceScale::MetersPerKilometer
                  << " km | "
                  << (nearBody ? "NEAR BODY" : "DISTANT BODY");
        }

        title << " | Jump: " << m_hyperdriveSystem.selectedTargetName(m_world);
        m_window.setTitle(title.str().c_str());
    }

    void GameApplication::render()
    {
        m_renderSystem.render(
            m_world,
            m_renderer,
            *m_renderResources,
            m_window.pixelWidth(),
            m_window.pixelHeight());
    }
}
