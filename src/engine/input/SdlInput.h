#pragma once

#include <SDL3/SDL.h>
#include <glm/vec2.hpp>

#include <array>

namespace SpaceSim
{
    enum class MouseButton
    {
        Left,
        Middle,
        Right
    };

    class SdlInput
    {
    public:
        void beginFrame();

        bool keyDown(SDL_Scancode key) const;
        bool keyPressed(SDL_Scancode key) const;
        bool keyReleased(SDL_Scancode key) const;

        bool mouseDown(MouseButton button) const;
        bool mousePressed(MouseButton button) const;
        bool mouseReleased(MouseButton button) const;

        glm::vec2 mouseDelta() const;

    private:
        static SDL_MouseButtonFlags mouseMask(MouseButton button);

        std::array<bool, SDL_SCANCODE_COUNT> m_currentKeys{};
        std::array<bool, SDL_SCANCODE_COUNT> m_previousKeys{};

        SDL_MouseButtonFlags m_currentMouseButtons = 0;
        SDL_MouseButtonFlags m_previousMouseButtons = 0;
        glm::vec2 m_mouseDelta{0.0f};
    };
}
