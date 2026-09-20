#include "input/SdlInput.h"

#include <algorithm>

namespace SpaceSim
{
    void SdlInput::beginFrame()
    {
        m_previousKeys = m_currentKeys;
        m_previousMouseButtons = m_currentMouseButtons;

        int keyCount = 0;
        const bool* keyboard = SDL_GetKeyboardState(&keyCount);

        m_currentKeys.fill(false);
        const int count = std::min(keyCount, static_cast<int>(SDL_SCANCODE_COUNT));
        for (int i = 0; i < count; ++i)
        {
            m_currentKeys[static_cast<std::size_t>(i)] = keyboard[i];
        }

        m_currentMouseButtons = SDL_GetMouseState(nullptr, nullptr);

        float deltaX = 0.0f;
        float deltaY = 0.0f;
        SDL_GetRelativeMouseState(&deltaX, &deltaY);
        m_mouseDelta = glm::vec2(deltaX, deltaY);
    }

    bool SdlInput::keyDown(SDL_Scancode key) const
    {
        const auto index = static_cast<std::size_t>(key);
        return index < m_currentKeys.size() && m_currentKeys[index];
    }

    bool SdlInput::keyPressed(SDL_Scancode key) const
    {
        const auto index = static_cast<std::size_t>(key);
        return index < m_currentKeys.size() &&
               m_currentKeys[index] &&
               !m_previousKeys[index];
    }

    bool SdlInput::keyReleased(SDL_Scancode key) const
    {
        const auto index = static_cast<std::size_t>(key);
        return index < m_currentKeys.size() &&
               !m_currentKeys[index] &&
               m_previousKeys[index];
    }

    SDL_MouseButtonFlags SdlInput::mouseMask(MouseButton button)
    {
        switch (button)
        {
        case MouseButton::Left:
            return SDL_BUTTON_LMASK;
        case MouseButton::Middle:
            return SDL_BUTTON_MMASK;
        case MouseButton::Right:
            return SDL_BUTTON_RMASK;
        }

        return 0;
    }

    bool SdlInput::mouseDown(MouseButton button) const
    {
        return (m_currentMouseButtons & mouseMask(button)) != 0;
    }

    bool SdlInput::mousePressed(MouseButton button) const
    {
        const SDL_MouseButtonFlags mask = mouseMask(button);
        return (m_currentMouseButtons & mask) != 0 &&
               (m_previousMouseButtons & mask) == 0;
    }

    bool SdlInput::mouseReleased(MouseButton button) const
    {
        const SDL_MouseButtonFlags mask = mouseMask(button);
        return (m_currentMouseButtons & mask) == 0 &&
               (m_previousMouseButtons & mask) != 0;
    }

    glm::vec2 SdlInput::mouseDelta() const
    {
        return m_mouseDelta;
    }
}
