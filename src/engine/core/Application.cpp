#include "core/Application.h"

#include "components/PlayerControlledComponent.h"
#include "components/ShipFlightComponent.h"
#include "components/TransformComponent.h"
#include "world/StarSystemLoader.h"

#include <raylib.h>
#include <raymath.h>

namespace SpaceSim
{
    int FindObjectIndexById(const StarSystem& system, const std::string& id)
    {
        for (int i = 0; i < static_cast<int>(system.objects.size()); ++i)
        {
            if (system.objects[i].id == id)
            {
                return i;
            }
        }

        return -1;
    }

    static const char* GetShipPresetName(ShipPreset preset)
    {
        switch (preset)
        {
        case ShipPreset::Light:
            return "Light";
        case ShipPreset::Medium:
            return "Medium";
        case ShipPreset::Heavy:
            return "Heavy";
        default:
            return "Unknown";
        }
    }
    Application::Application()
    {
        m_renderer = std::make_unique<Renderer>(1280, 720, "SpaceSim");

        m_world.starSystem = LoadStarSystemFromJson("data/systems/test_system.json");
        m_world.selectedJumpTarget = FindObjectIndexById(m_world.starSystem, "aster_relay");
        m_world.activePoi = m_world.selectedJumpTarget;
        m_world.activeBubbleOrigin = m_world.starSystem.objects[m_world.activePoi].position;
        m_world.globalPlayerPosition = m_world.activeBubbleOrigin;

        createPlayerShip();

        auto& transform = m_world.registry.get<TransformComponent>(m_world.playerShip);
        transform.position = Vector3{ 0.0f, 0.0f, -300.0f };

        m_cameraSystem.initialize(m_world);
        DisableCursor();
    }

    Application::~Application() = default;

    int Application::run()
    {
        while (!m_renderer->shouldClose())
        {
            const float dt = GetFrameTime();

            update(dt);
            render();
        }

        return 0;
    }

    void Application::createPlayerShip()
    {
        auto ship = m_world.registry.create();

        m_world.registry.emplace<TransformComponent>(ship);
        m_world.registry.emplace<ShipFlightComponent>(ship);
        m_world.registry.emplace<PlayerControlledComponent>(ship);

        m_world.playerShip = ship;
    }

    void Application::update(float dt)
    {
        if (IsKeyPressed(KEY_F11))
        {
            if (IsWindowFullscreen())
            {
                ToggleFullscreen();
                SetWindowSize(1280, 720);
            }
            else
            {
                const int monitor = GetCurrentMonitor();
                SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
                ToggleFullscreen();
            }
        }
        m_ftlSystem.update(m_world, dt);

        if (m_world.travelMode == TravelMode::NormalFlight)
        {
            m_shipControlSystem.update(m_world, dt);
        }

        const auto& transform = m_world.registry.get<TransformComponent>(m_world.playerShip);

        constexpr double localToGlobalScale = 0.001;

        if (m_world.travelMode == TravelMode::NormalFlight)
        {
            m_world.globalPlayerPosition = {
                m_world.activeBubbleOrigin.x + transform.position.x * localToGlobalScale,
                m_world.activeBubbleOrigin.y + transform.position.y * localToGlobalScale,
                m_world.activeBubbleOrigin.z + transform.position.z * localToGlobalScale
            };
        }

        m_cameraSystem.update(m_world, dt);
    }

    void Application::render()
    {
        m_renderer->beginFrame();

        m_renderSystem.renderSky(m_world, *m_renderer);

        m_renderer->begin3D(m_world.camera);
        m_renderSystem.renderWorld(m_world, *m_renderer);
        m_renderer->end3D();
        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();

        const float centerX = static_cast<float>(screenWidth) * 0.5f;
        const float centerY = static_cast<float>(screenHeight) * 0.5f;

        const float controlRadius = m_shipControlSystem.getControlRadius();
        const Vector2 virtualStick = m_shipControlSystem.getVirtualStick();

        if (m_world.travelMode == TravelMode::NormalFlight)
        {
            DrawCircleLines(centerX, centerY, controlRadius, DARKGRAY);
            DrawCircle(centerX, centerY, 3.0f, RAYWHITE);
            DrawCircle(
                static_cast<int>(centerX + virtualStick.x),
                static_cast<int>(centerY + virtualStick.y),
                5.0f,
                SKYBLUE
            );
        }
        m_renderer->drawDebugText("SpaceSim - EnTT ECS active", 20, 20);
        m_renderer->drawDebugText("W/S = throttle, Z = zero, A/D = strafe, Space/Ctrl = up/down, Q/E = roll", 20, 45);

        auto& flight = m_world.registry.get<ShipFlightComponent>(m_world.playerShip);

        DrawText(
            TextFormat(
                "Thrust Mode: %s [T]",
                flight.holdThrustMode ? "HOLD" : "THROTTLE"
            ),
            20,
            70,
            20,
            RAYWHITE
        );

        DrawText(
            TextFormat(
                "Throttle: %.0f%%  W/S  Z = zero",
                flight.throttle * 100.0f
            ),
            20,
            95,
            20,
            RAYWHITE
        );

        float speed = Vector3Length(flight.velocity);

        DrawText(
            TextFormat("Speed: %.1f / %.1f", speed, flight.maxSpeed),
            20,
            120,
            20,
            RAYWHITE
        );

        DrawText(
            flight.flightAssist ? "Flight Assist: ON  [X]" : "Flight Assist: OFF [X]",
            20,
            145,
            20,
            flight.flightAssist ? GREEN : ORANGE
        );

        DrawText(
            TextFormat("Ship Preset: %s  [1/2/3]", GetShipPresetName(flight.preset)),
            20,
            170,
            20,
            RAYWHITE
        );
                if (m_world.selectedJumpTarget >= 0)
        {
            const auto& target = m_world.starSystem.objects[m_world.selectedJumpTarget];
            const DVec3 toTarget = target.position - m_world.globalPlayerPosition;

            DrawText(
                TextFormat(
                    "FTL Target: %s  Distance: %.0f  [Tab] cycle  [J] travel",
                    target.name.c_str(),
                    Length(toTarget)
                ),
                20,
                195,
                20,
                YELLOW
            );
            if (m_world.travelMode == TravelMode::FTLTravel)
                {
                    const double distanceToTarget = Length(
                        m_world.ftlTravel.destination - m_world.globalPlayerPosition
                    );

                    const bool charging =
                        m_world.ftlTravel.chargeTimer < m_world.ftlTravel.chargeTime;

                    if (charging)
                    {
                        DrawText(
                            TextFormat(
                                "FTL ALIGNING  Distance: %.0f  [C] cancel",
                                distanceToTarget
                            ),
                            20,
                            270,
                            20,
                            YELLOW
                        );
                    }
                    else
                    {
                        DrawText(
                            TextFormat(
                                "FTL TRAVEL ACTIVE  Distance: %.0f  Speed: %.0f  [C] drop out",
                                distanceToTarget,
                                m_world.ftlTravel.speed
                            ),
                            20,
                            270,
                            20,
                            SKYBLUE
                        );
                    }
                }
        }

        if (m_world.activePoi >= 0)
        {
            const auto& activePoi = m_world.starSystem.objects[m_world.activePoi];

            DrawText(
                TextFormat("Nearby POI: %s", activePoi.name.c_str()),
                20,
                220,
                20,
                SKYBLUE
            );
        }
        else
        {
            DrawText(
                TextFormat(
                    "Bubble Origin: %.0f %.0f %.0f",
                    m_world.activeBubbleOrigin.x,
                    m_world.activeBubbleOrigin.y,
                    m_world.activeBubbleOrigin.z
                ),
                20,
                220,
                20,
                SKYBLUE
            );
        }

        DrawText(
            TextFormat(
                "Global Pos: %.0f %.0f %.0f",
                m_world.globalPlayerPosition.x,
                m_world.globalPlayerPosition.y,
                m_world.globalPlayerPosition.z
            ),
            20,
            295,
            20,
            RAYWHITE
        );
        m_renderer->endFrame();
    }
}