# ECS flight slice handoff

This build contains the first player-controlled ship plus the first physical scale/coordinate pass.

## Runtime flow

`SdlInput` → `PlayerControlSystem` → `ShipControlComponent` → 120 Hz `ShipMovementSystem` → `TransformComponent` → `ChaseCameraSystem` / `RenderSystem`

The movement system has no SDL dependency. Any later AI/autopilot/controller layer can write `ShipControlComponent` and reuse the same flight model.

## Scale convention

Local gameplay positions are metres and velocities are metres/second. Large bodies use double-precision global metre positions. Rendering is camera-relative and is allowed to choose a different float scale.

The placeholder ship is approximately 28 m long. The prototype planet is physically Earth-scale and the flight bubble begins six planetary radii from its center. The renderer keeps the planet at a fake safe distance while preserving its true apparent angular size.

See `SCALE_COORDINATES_V1.md` for details.

## Current controls

- W / S: forward / reverse
- A / D: strafe left / right
- Space / Left Ctrl: vertical thrust
- Hold RMB + mouse: pitch / yaw
- Arrow keys: keyboard pitch / yaw
- Q / E: roll
- Shift: boost
- F: flight assist toggle
- F11: fullscreen
- Escape: quit

## Tuning

Initial physical flight values live in `src/game/ecs/components/ShipMovementComponent.h`. The title bar shows current speed in m/s and km/h.

Known-size debug cubes are present in the prototype scene so flight can be tuned against local scale rather than against a planet alone.

## Verification note

Build and run `SpaceSim`. `RendererLab` remains a separate renderer test executable.
