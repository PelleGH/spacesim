# Modern runtime architecture

## Runtime ownership

RendererLab grew into a monolithic host while the new renderer was being developed. It remains a renderer regression/test executable, but it no longer owns gameplay.

The actual game runtime is now:

`src/main.cpp`
→ `GameApplication`
→ SDL window/input/time
→ EnTT `GameWorld`
→ gameplay systems
→ `RenderSystem`
→ `SceneRenderer`

Both `SpaceSim` and `RendererLab` link the same `SpaceSimRenderer` library.

## Current game loop

`GameApplication` has explicit phases:

1. Poll SDL events and sample input once per rendered frame.
2. `PlayerControlSystem` writes renderer-independent ship control intent into ECS.
3. Run zero or more 120 Hz `fixedUpdate()` ticks.
4. `ShipMovementSystem` integrates ship translation/rotation during fixed ticks.
5. `ChaseCameraSystem` updates presentation at render rate.
6. `RenderSystem` extracts ECS/global/local state and calls the shared renderer.

Simulation that affects gameplay/physics belongs in the fixed phase. Camera smoothing and other presentation-only work may run at render rate.

## Coordinate layers

The modern game runtime now has an explicit first-pass scale split:

### Global layer

Large bodies use double-precision physical positions in metres through `GlobalPositionComponent`.

### Local gameplay bubble

`TransformComponent::positionMeters` is local physical position in metres. `GameWorld::localBubbleOriginMeters` says which global point local `(0,0,0)` represents. Movement uses m/s and m/s^2.

### Render layer

`RenderSystem` performs camera-relative extraction. Nearby local objects are converted at 100 m per render unit. Distant planets are placed at a safe fake render distance and scaled from their real radius/distance ratio, preserving angular size without using astronomical float coordinates.

See `SCALE_COORDINATES_V1.md` for the current conventions.

## First flight slice

The prototype world spawns a player ship entity with:

- `TransformComponent`
- `PlayerControlledComponent`
- `ShipControlComponent`
- `ShipMovementComponent`
- `RenderableComponent`

`PlayerControlSystem` is the only ship system that knows about SDL. `ShipMovementSystem` consumes only `ShipControlComponent`, which means AI/autopilot/controller/replay inputs can drive the same movement model later.

The current ship is intentionally a renderer-side placeholder made from a few boxes. Its temporary dimensions are now specified in physical metres during render extraction. ECS stores only a renderer-neutral `RenderMeshKind`; the actual `GpuMesh` remains in `GameRenderResources`.

## ECS/render boundary

ECS components may contain simulation data and renderer-neutral visual parameters, but not GPU resources. Shared meshes, atmosphere LUTs, cubemaps and IBL resources are owned by `GameRenderResources`.

`RenderSystem` creates the renderer-facing structures already used by the modern pipeline. Renderer-only concepts such as fake distant-body distance and metres-to-render-unit conversion do not leak back into gameplay components.

## Controls in the current prototype

- `W / S` — forward / reverse thrust
- `A / D` — strafe left / right
- `Space / Left Ctrl` — thrust up / down
- Hold `RMB` + mouse — pitch / yaw
- Arrow keys — keyboard pitch / yaw
- `Q / E` — roll
- `Shift` — boost
- `F` — toggle flight assist
- `F11` — fullscreen
- `Escape` — quit

Flight values remain plain fields on `ShipMovementComponent` for rapid tuning.

## Legacy source

The old Raylib application and the `SpaceSimNext` integration experiment are intentionally not built by the current CMake configuration. They remain in the tree as reference material while useful systems are reimplemented using SDL3/GLM and the modern renderer.
