# Space Sim hobby project

A hobby space-sim project with a custom SDL3/OpenGL renderer and an EnTT-based game layer.

## Screenshots

<img width="2096" height="967" alt="image" src="https://github.com/user-attachments/assets/6e9ea502-396e-4b51-825f-3b7d586dff6d" />

<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/a051dc3a-425a-4f85-bdab-d0ef7a100e23" />

<img width="2554" height="1440" alt="image" src="https://github.com/user-attachments/assets/2d00ea37-a026-44b5-96e5-d2185bec99a3" />

<img width="2072" height="1393" alt="image" src="https://github.com/user-attachments/assets/f31e9159-833f-4769-bfa3-aff6a4ca01f0" />
Now with clouds! (Soon™)

## Current executables

- `SpaceSim` — the actual game runtime. SDL3/OpenGL application loop, EnTT world, game systems, and render extraction.

- cmake -S . -B build
- cmake --build build --config Release

- `RendererLab` — renderer regression/test harness. It uses the same shared renderer as the game but keeps the old rendering test scene and `--ocean-check` workflow.

The older Raylib gameplay/integration sources are still present as reference while useful ideas are migrated, but they are no longer part of the normal CMake build.

## Architecture

The game owns simulation state in EnTT. Gameplay systems update that state. `RenderSystem` extracts renderer-facing data (`RenderCamera`, `PlanetRenderObject`, lights, atmosphere instance) and passes it to `SceneRenderer`.

## Third-party dependencies used by the normal build

- SDL3 — windowing and platform input
- glad — OpenGL function loading
- GLM — math
- EnTT — ECS
- CMake — build configuration

See `THIRD_PARTY_NOTICES.md` for license information.
