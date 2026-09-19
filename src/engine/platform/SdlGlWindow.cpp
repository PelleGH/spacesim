#include "platform/SdlGlWindow.h"

#include <glad/gl.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace SpaceSim
{
    SdlGlWindow::SdlGlWindow(
        int width,
        int height,
        const char* title)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            throw std::runtime_error(
                std::string("SDL_Init failed: ") +
                SDL_GetError());
        }

        // Request a modern OpenGL Core context.
        if (!SDL_GL_SetAttribute(
                SDL_GL_CONTEXT_MAJOR_VERSION, 4) ||
            !SDL_GL_SetAttribute(
                SDL_GL_CONTEXT_MINOR_VERSION, 5) ||
            !SDL_GL_SetAttribute(
                SDL_GL_CONTEXT_PROFILE_MASK,
                SDL_GL_CONTEXT_PROFILE_CORE))
        {
            throw std::runtime_error(
                std::string("Failed to set OpenGL attributes: ") +
                SDL_GetError());
        }

        SDL_GL_SetAttribute(
            SDL_GL_DOUBLEBUFFER,
            1);

        SDL_GL_SetAttribute(
            SDL_GL_DEPTH_SIZE,
            24);

        SDL_GL_SetAttribute(
            SDL_GL_STENCIL_SIZE,
            8);

        // Eventually the tone-mapped linear image will be converted
        // to the monitor's sRGB representation here.
        SDL_GL_SetAttribute(
            SDL_GL_FRAMEBUFFER_SRGB_CAPABLE,
            1);

        m_window = SDL_CreateWindow(
            title,
            width,
            height,
            SDL_WINDOW_OPENGL |
            SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_HIGH_PIXEL_DENSITY);

        if (!m_window)
        {
            throw std::runtime_error(
                std::string("SDL_CreateWindow failed: ") +
                SDL_GetError());
        }

        m_context = SDL_GL_CreateContext(m_window);

        if (!m_context)
        {
            throw std::runtime_error(
                std::string("SDL_GL_CreateContext failed: ") +
                SDL_GetError());
        }

        if (!SDL_GL_MakeCurrent(
                m_window,
                m_context))
        {
            throw std::runtime_error(
                std::string("SDL_GL_MakeCurrent failed: ") +
                SDL_GetError());
        }

        const int loadedVersion =
            gladLoadGL(
                reinterpret_cast<GLADloadfunc>(
                    SDL_GL_GetProcAddress));

        if (loadedVersion == 0)
        {
            throw std::runtime_error(
                "GLAD failed to load OpenGL.");
        }

        std::cout
            << "OpenGL vendor: "
            << glGetString(GL_VENDOR)
            << '\n';

        std::cout
            << "OpenGL renderer: "
            << glGetString(GL_RENDERER)
            << '\n';

        std::cout
            << "OpenGL version: "
            << glGetString(GL_VERSION)
            << '\n';

        // Enable vsync if available.
        if (!SDL_GL_SetSwapInterval(1))
        {
            std::cerr
                << "Warning: VSync unavailable: "
                << SDL_GetError()
                << '\n';
        }

        // These will become standard renderer state.
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    SdlGlWindow::~SdlGlWindow()
    {
        if (m_context)
        {
            SDL_GL_DestroyContext(m_context);
            m_context = nullptr;
        }

        if (m_window)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_Quit();
    }

    bool SdlGlWindow::processEvents()
    {
        SDL_Event event{};

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                m_running = false;
            }

            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.key == SDLK_ESCAPE)
            {
                m_running = false;
            }
        }

        return m_running;
    }

    void SdlGlWindow::swapBuffers()
    {
        SDL_GL_SwapWindow(m_window);
    }

    int SdlGlWindow::pixelWidth() const
    {
        int width = 0;
        int height = 0;

        SDL_GetWindowSizeInPixels(
            m_window,
            &width,
            &height);

        return width;
    }

    int SdlGlWindow::pixelHeight() const
    {
        int width = 0;
        int height = 0;

        SDL_GetWindowSizeInPixels(
            m_window,
            &width,
            &height);

        return height;
    }
}