#pragma once

#include <raylib.h>

namespace SpaceSim
{
    class Renderer
    {
    public:
        Renderer(int width, int height, const char* title);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        bool shouldClose() const;

        void beginFrame();
        void begin3D(const Camera3D& camera);
        void end3D();
        void endFrame();

        void drawTestScene();
        void drawDebugText(const char* text, int x, int y);

    private:
        int m_width = 0;
        int m_height = 0;
    };
}