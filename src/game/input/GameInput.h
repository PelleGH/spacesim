#pragma once
#include <raylib.h>
namespace SpaceSim::GameInput {
#ifdef SPACESIM_SDL_GAME
void beginFrame(bool captureMouse);
bool keyDown(int key);
bool keyPressed(int key);
Vector2 mouseDelta();
// Deterministic input for integration tests, no synthetic OS input.
void inject(int key,bool held);
void clear();
#else
inline bool keyDown(int key) { return IsKeyDown(key); }
inline bool keyPressed(int key) { return IsKeyPressed(key); }
inline Vector2 mouseDelta() { return GetMouseDelta(); }
#endif
}
