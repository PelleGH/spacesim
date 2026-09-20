#include "game/systems/PlayerControlSystem.h"

#include "game/ecs/components/HyperdriveComponents.h"
#include "game/ecs/components/PlayerControlledComponent.h"
#include "game/ecs/components/ShipControlComponent.h"
#include "input/SdlInput.h"

#include <SDL3/SDL.h>
#include <glm/common.hpp>

#include <algorithm>
#include <iostream>

namespace SpaceSim
{
    namespace
    {
        float axis(bool positive, bool negative)
        {
            return (positive ? 1.0f : 0.0f) - (negative ? 1.0f : 0.0f);
        }
    }

    void PlayerControlSystem::update(GameWorld& world, const SdlInput& input, float frameDt)
    {
        const float safeFrameDt = std::max(frameDt, 1.0f / 500.0f);
        constexpr float mouseRadiansPerPixel = 0.0022f;
        constexpr float maximumMouseRate = 3.0f;

        const auto view = world.registry.view<
            PlayerControlledComponent,
            ShipControlComponent,
            HyperdriveControlComponent>();
        for (const entt::entity entity : view)
        {
            auto& control = view.get<ShipControlComponent>(entity);
            auto& hyperdriveControl = view.get<HyperdriveControlComponent>(entity);

            hyperdriveControl.cycleTargetRequested = input.keyPressed(SDL_SCANCODE_TAB);
            hyperdriveControl.engageRequested = input.keyPressed(SDL_SCANCODE_J);

            control.linearInput = glm::vec3(
                axis(input.keyDown(SDL_SCANCODE_D), input.keyDown(SDL_SCANCODE_A)),
                axis(input.keyDown(SDL_SCANCODE_SPACE), input.keyDown(SDL_SCANCODE_LCTRL)),
                axis(input.keyDown(SDL_SCANCODE_W), input.keyDown(SDL_SCANCODE_S)));

            control.angularInput = glm::vec3(
                axis(input.keyDown(SDL_SCANCODE_UP), input.keyDown(SDL_SCANCODE_DOWN)),
                axis(input.keyDown(SDL_SCANCODE_RIGHT), input.keyDown(SDL_SCANCODE_LEFT)),
                axis(input.keyDown(SDL_SCANCODE_E), input.keyDown(SDL_SCANCODE_Q)));

            control.boost = input.keyDown(SDL_SCANCODE_LSHIFT) || input.keyDown(SDL_SCANCODE_RSHIFT);

            if (input.keyPressed(SDL_SCANCODE_F))
            {
                control.flightAssist = !control.flightAssist;
                std::cout << "Flight assist: " << (control.flightAssist ? "ON" : "OFF") << '\n';
            }

            control.mouseSteering = input.mouseDown(MouseButton::Right);
            control.mouseLookRate = glm::vec2(0.0f);

            if (control.mouseSteering)
            {
                const glm::vec2 delta = input.mouseDelta();
                control.mouseLookRate.x = glm::clamp(
                    -delta.y * mouseRadiansPerPixel / safeFrameDt,
                    -maximumMouseRate,
                    maximumMouseRate);
                control.mouseLookRate.y = glm::clamp(
                    delta.x * mouseRadiansPerPixel / safeFrameDt,
                    -maximumMouseRate,
                    maximumMouseRate);
            }
        }
    }
}
