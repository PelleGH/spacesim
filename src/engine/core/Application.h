#pragma once

#include "renderer/Renderer.h"
#include "systems/CameraSystem.h"
#include "systems/RenderSystem.h"
#include "systems/ShipControlSystem.h"
#include "world/GameWorld.h"
#include "systems/FTLSystem.h"
#include "systems/StarSystemSystem.h"
#include "systems/BubbleSystem.h"

#include <memory>

namespace SpaceSim
{
    class Application
    {
    public:
        Application();
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        int run();

    private:
        void createPlayerShip();

        void update(float dt);
        void render();

    private:
        std::unique_ptr<Renderer> m_renderer;

        GameWorld m_world;

        ShipControlSystem m_shipControlSystem;
        CameraSystem m_cameraSystem;
        RenderSystem m_renderSystem;
        FTLSystem m_ftlSystem;
        StarSystemSystem m_starSystemSystem;
        BubbleSystem m_bubbleSystem;
        bool m_mouseCaptured = true;
    };
}