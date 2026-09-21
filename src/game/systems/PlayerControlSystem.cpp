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
            return
                (positive ? 1.0f : 0.0f)
                -
                (negative ? 1.0f : 0.0f);
        }
    }

    void PlayerControlSystem::update(
        GameWorld& world,
        const SdlInput& input,
        float frameDt)
    {
        const float safeFrameDt =
            std::max(
                frameDt,
                1.0f / 500.0f);

        constexpr float mouseRadiansPerPixel =
            0.0022f;

        constexpr float maximumMouseRate =
            3.0f;

        const auto view =
            world.registry.view<
                PlayerControlledComponent,
                ShipControlComponent,
                HyperdriveComponent,
                HyperdriveControlComponent>();

        for (const entt::entity entity : view)
        {
            auto& control =
                view.get<ShipControlComponent>(
                    entity);

            const auto& hyperdrive =
                view.get<HyperdriveComponent>(
                    entity);

            auto& hyperdriveControl =
                view.get<HyperdriveControlComponent>(
                    entity);


            // ---------------------------------------------------------
            // HYPERDRIVE
            // ---------------------------------------------------------

            hyperdriveControl.cycleTargetRequested =
                input.keyPressed(
                    SDL_SCANCODE_TAB);

            hyperdriveControl.engageRequested =
                input.keyPressed(
                    SDL_SCANCODE_J);


            // ---------------------------------------------------------
            // ORBITAL CRUISE
            // ---------------------------------------------------------

            if (input.keyPressed(SDL_SCANCODE_C))
            {
                if (hyperdrive.state == HyperdriveState::Idle)
                {
                    control.orbitalCruise =
                        !control.orbitalCruise;

                    std::cout
                        << "Orbital cruise: "
                        << (
                            control.orbitalCruise
                                ? "ON | 50 km/s cruise, 100 km/s boost"
                                : "OFF"
                        )
                        << '\n';
                }
                else
                {
                    std::cout
                        << "Orbital cruise unavailable during hyperdrive.\n";
                }
            }

            // Hyperdrive owns translation while active.
            if (hyperdrive.state != HyperdriveState::Idle)
            {
                control.orbitalCruise =
                    false;
            }


            // ---------------------------------------------------------
            // TRANSLATION
            // ---------------------------------------------------------

            control.linearInput =
                glm::vec3(
                    axis(
                        input.keyDown(SDL_SCANCODE_D),
                        input.keyDown(SDL_SCANCODE_A)),

                    axis(
                        input.keyDown(SDL_SCANCODE_SPACE),
                        input.keyDown(SDL_SCANCODE_LCTRL)),

                    axis(
                        input.keyDown(SDL_SCANCODE_W),
                        input.keyDown(SDL_SCANCODE_S)));


            // ---------------------------------------------------------
            // ROTATION
            // ---------------------------------------------------------

            control.angularInput =
                glm::vec3(
                    axis(
                        input.keyDown(SDL_SCANCODE_UP),
                        input.keyDown(SDL_SCANCODE_DOWN)),

                    axis(
                        input.keyDown(SDL_SCANCODE_RIGHT),
                        input.keyDown(SDL_SCANCODE_LEFT)),

                    axis(
                        input.keyDown(SDL_SCANCODE_E),
                        input.keyDown(SDL_SCANCODE_Q)));


            control.boost =
                input.keyDown(SDL_SCANCODE_LSHIFT)
                ||
                input.keyDown(SDL_SCANCODE_RSHIFT);


            // ---------------------------------------------------------
            // FLIGHT ASSIST
            // ---------------------------------------------------------

            if (input.keyPressed(SDL_SCANCODE_F))
            {
                control.flightAssist =
                    !control.flightAssist;

                std::cout
                    << "Flight assist: "
                    << (
                        control.flightAssist
                            ? "ON"
                            : "OFF"
                    )
                    << '\n';
            }


            // ---------------------------------------------------------
            // MOUSE STEERING
            // ---------------------------------------------------------

            control.mouseSteering =
                input.mouseDown(
                    MouseButton::Right);

            control.mouseLookRate =
                glm::vec2(
                    0.0f);

            if (control.mouseSteering)
            {
                const glm::vec2 delta =
                    input.mouseDelta();

                control.mouseLookRate.x =
                    glm::clamp(
                        -delta.y
                            *
                            mouseRadiansPerPixel
                            /
                            safeFrameDt,

                        -maximumMouseRate,
                        maximumMouseRate);

                control.mouseLookRate.y =
                    glm::clamp(
                        delta.x
                            *
                            mouseRadiansPerPixel
                            /
                            safeFrameDt,

                        -maximumMouseRate,
                        maximumMouseRate);
            }
        }
    }
}