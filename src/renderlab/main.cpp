#include "platform/SdlGlWindow.h"

#include <glad/gl.h>

#include <exception>
#include <iostream>

int main()
{
    try
    {
        SpaceSim::SdlGlWindow window(
            1280,
            720,
            "SpaceSim Renderer Lab");

        while (window.processEvents())
        {
            const int width =
                window.pixelWidth();

            const int height =
                window.pixelHeight();

            glViewport(
                0,
                0,
                width,
                height);

            // Almost-black space-like background.
            glClearColor(
                0.003f,
                0.004f,
                0.008f,
                1.0f);

            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT);

            window.swapBuffers();
        }

        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr
            << "RendererLab fatal error: "
            << exception.what()
            << '\n';

        return 1;
    }
}