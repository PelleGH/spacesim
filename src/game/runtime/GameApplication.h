#pragma once

#include "core/GameClock.h"
#include "game/ecs/GameWorld.h"
#include "game/rendering/RenderSystem.h"
#include "game/systems/ChaseCameraSystem.h"
#include "game/systems/HyperdriveSystem.h"
#include "game/systems/LocalBubbleRebaseSystem.h"
#include "game/systems/PlayerControlSystem.h"
#include "game/systems/ShipMovementSystem.h"
#include "input/SdlInput.h"
#include "platform/SdlGlWindow.h"
#include "renderer/SceneRenderer.h"

#include <memory>

namespace SpaceSim
{
    class GameRenderResources;

    class GameApplication
    {
    public:
        GameApplication();
        ~GameApplication();

        GameApplication(const GameApplication&) = delete;
        GameApplication& operator=(const GameApplication&) = delete;

        int run();

    private:
        void handleApplicationInput();
        void sampleGameplayInput(float frameDt);
        void fixedUpdate(float dt);
        void update(float dt);
        void updateDebugTitle(float dt);
        void render();

        static constexpr double FixedDeltaSeconds = 1.0 / 120.0;
        static constexpr double MaxFrameDeltaSeconds = 0.1;

        // Window must exist before any OpenGL-backed renderer/resources.
        SdlGlWindow m_window;
        SceneRenderer m_renderer;

        SdlInput m_input;
        GameClock m_clock;
        GameWorld m_world;

        PlayerControlSystem m_playerControlSystem;
        ShipMovementSystem m_shipMovementSystem;
        LocalBubbleRebaseSystem m_localBubbleRebaseSystem;
        HyperdriveSystem m_hyperdriveSystem;
        ChaseCameraSystem m_chaseCameraSystem;
        RenderSystem m_renderSystem;
        std::unique_ptr<GameRenderResources> m_renderResources;

        double m_accumulator = 0.0;
        double m_titleUpdateAccumulator = 0.0;
    };
}
