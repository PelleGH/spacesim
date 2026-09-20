#pragma once

#include <SDL3/SDL.h>

namespace SpaceSim
{
    class SdlGlWindow
    {
    public:
        SdlGlWindow(
            int width,
            int height,
            const char* title);

        ~SdlGlWindow();

        SdlGlWindow(const SdlGlWindow&) = delete;
        SdlGlWindow& operator=(const SdlGlWindow&) = delete;

        bool processEvents();

        void swapBuffers();

        void captureMouse(bool capture);
        void toggleFullscreen();
        int pixelWidth() const;
        int pixelHeight() const;

    private:
        SDL_Window* m_window = nullptr;
        SDL_GLContext m_context = nullptr;

        bool m_running = true;
    };
}