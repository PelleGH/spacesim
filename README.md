# Space Sim hobby project

A hobby space-sim project with a custom SDL3/OpenGL renderer and an EnTT-based game layer.

## Current executables

- `SpaceSim` — the actual game runtime. SDL3/OpenGL application loop, EnTT world, game systems, and render extraction.
- `RendererLab` — renderer regression/test harness. It uses the same shared renderer as the game but keeps the old rendering test scene and `--ocean-check` workflow.

The older Raylib gameplay/integration sources are still present as reference while useful ideas are migrated, but they are no longer part of the normal CMake build.

## Architecture

The game owns simulation state in EnTT. Gameplay systems update that state. `RenderSystem` extracts renderer-facing data (`RenderCamera`, `PlanetRenderObject`, lights, atmosphere instance) and passes it to `SceneRenderer`. GPU objects stay outside ECS components.

The application loop has explicit phases:

- frame input sampling into ECS control intent
- fixed simulation updates at 120 Hz (`GameApplication::fixedUpdate`)
- per-frame presentation systems (`GameApplication::update`)
- render extraction into `SceneRenderer`

The current prototype includes the first player ship flight slice: 6DOF local thrust, boost, flight-assist damping, mouse/keyboard rotation, a chase camera, and a renderer-side placeholder ship. See `ARCHITECTURE_REFACTOR.md` for controls and system boundaries.

## Third-party dependencies used by the normal build

- SDL3 — windowing and platform input
- glad — OpenGL function loading
- GLM — math
- EnTT — ECS
- CMake — build configuration

See `THIRD_PARTY_NOTICES.md` for license information.
