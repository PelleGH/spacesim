# Planet Near-Body Handoff v1

This pass fixes the point where the distant-body representation previously made a planet surface visually unreachable.

## Coordinate truth

Gameplay/world state remains physical:

- `GlobalPositionComponent` is double-precision metres.
- local `TransformComponent::positionMeters` is metres.
- `GameWorld::localBubbleOriginMeters` is the global position represented by local `(0,0,0)`.
- normal ship movement therefore changes true global distance to a planet.

## Render modes

`RenderSystem` now derives planet altitude from the camera's true global position.

### Distant body

Above 1500 km altitude the existing compact representation remains in use:

- center at a safe fake render distance;
- render radius derived from physical `planetRadius / centerDistance`;
- apparent angular size is preserved;
- no astronomical float coordinates are sent to the renderer.

### Near body

At or below 1500 km altitude the planet switches to a true camera-relative representation:

- center = `(planetGlobal - cameraGlobal) * RenderUnitsPerLocalMeter`;
- radius = `planetRadiusMeters * RenderUnitsPerLocalMeter`;
- the same scale is used by ships and other local gameplay objects;
- the old `0.98` radius/distance clamp is gone;
- the renderer far plane expands dynamically to contain the physical planet and atmosphere.

The modern renderer already uses reversed-Z for its HDR scene depth, so the larger near-body depth range is intentional.

Hyperdrive currently exits Earth travel at 120 km altitude, which means it exits directly into Near Body mode. Normal thrust can then reduce the actual global altitude continuously toward zero.

## Local bubble rebasing

`LocalBubbleRebaseSystem` runs after normal movement. When the player is 10 km or more from local origin it:

1. adds the player's local offset to `localBubbleOriginMeters`;
2. subtracts the same offset from every local `TransformComponent`.

This preserves all global and relative positions while keeping the active simulation numerically close to local zero.

## Debug telemetry

The window title now shows:

- ship speed / hyperdrive state;
- true altitude above the primary planet in km;
- `DISTANT BODY` or `NEAR BODY`;
- selected jump target.

## Not implemented yet

This is not the final surface renderer. Below roughly 100 km the next step is a planet-local tangent/surface mode using adaptive terrain/ocean patches. There is also no planet-surface collision or landing logic yet, so the current near-body sphere can still be flown through after altitude reaches zero.
