#include "renderer/Renderer.h"

namespace SpaceSim
{
    Renderer::Renderer(int width, int height, const char* title)
        : m_width(width), m_height(height)
    {
        InitWindow(m_width, m_height, title);
        SetTargetFPS(60);
    }

    Renderer::~Renderer()
    {
        CloseWindow();
    }

    bool Renderer::shouldClose() const
    {
        return WindowShouldClose();
    }

    void Renderer::beginFrame()
    {
        BeginDrawing();
        ClearBackground(Color{ 5, 5, 15, 255 });
    }

    void Renderer::begin3D(const Camera3D& camera)
    {
        BeginMode3D(camera);
    }

    void Renderer::end3D()
    {
        EndMode3D();
    }

    void Renderer::endFrame()
    {
        EndDrawing();
    }

    void Renderer::drawTestScene()
    {
        DrawGrid(20, 1.0f);

        DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, 1.0f, 1.0f, BLUE);
        DrawCubeWires(Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f, 1.0f, 1.0f, WHITE);

        DrawSphere(Vector3{ 3.0f, 0.0f, 0.0f }, 0.5f, GRAY);
    }

    void Renderer::drawDebugText(const char* text, int x, int y)
    {
        DrawText(text, x, y, 20, RAYWHITE);
    }
}