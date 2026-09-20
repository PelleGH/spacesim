# Scale / Coordinates v1

This pass separates physical gameplay scale from renderer scale before further flight tuning.

## Physical gameplay convention

Local gameplay uses SI-style physical units:

- local position: metres
- local velocity: metres/second
- acceleration: metres/second^2
- planet radius/global distance: metres (double precision)
- atmosphere renderer data remains kilometres because the atmosphere implementation is already authored in km

`TransformComponent::positionMeters` is local to the active gameplay bubble. `GameWorld::localBubbleOriginMeters` is the double-precision global position represented by local `(0,0,0)`.

For now the bubble origin is fixed. A later floating-origin/rebase system can move the origin without changing the local simulation model.

## Global bodies

Large-scale bodies use `GlobalPositionComponent` instead of a local `TransformComponent` for placement.

The prototype Earth-like planet is physically Earth-scale and sits at global origin. The local flight bubble starts six planet radii away from its center. The player therefore starts in deep/orbital space instead of a few arbitrary renderer units above the ocean.

## Render extraction

The renderer remains float-based and is deliberately decoupled from gameplay scale.

Nearby gameplay objects are extracted camera-relative at:

`100 physical metres = 1 render unit`

The render camera stays at float `(0,0,0)`. Local object positions are computed relative to the camera before conversion to floats.

The distant planet does not get placed at its true tens-of-thousands-of-kilometres distance. `RenderSystem` places its center at a fixed safe render distance and computes render radius from the true physical `planetRadius / cameraDistance` ratio. This preserves apparent angular size while avoiding astronomical float coordinates.

The planet renderer still receives its physical radius in kilometres for atmosphere/ocean calculations.

## Flight scale

The placeholder ship is now approximately 28 metres long. Initial flight values are physical/tunable values:

- forward acceleration: 28 m/s^2
- reverse acceleration: 18 m/s^2
- strafe/vertical acceleration: 16 m/s^2
- normal speed envelope: 180 m/s
- boost speed envelope: 420 m/s
- boost acceleration multiplier: 2.6x

The chase camera is approximately 42 m behind and 7.5 m above the ship.

The window title reports current ship speed in m/s and km/h plus flight-assist state.

## Temporary scale references

The prototype scene contains known-size debug cubes:

- 10 m cube at about 132 m
- 25 m cube at about 351 m
- 100 m cube at about 919 m
- 250 m cube at about 2.22 km

These are temporary calibration objects for judging speed, camera distance and ship scale. They should disappear once real stations, asteroids and ship assets provide trustworthy scale cues.

## Deliberately not solved yet

This is only the first coordinate architecture pass. It does not yet implement:

- local bubble rebasing / floating origin
- seamless near-planet/surface representation switching
- orbital mechanics
- orbital/travel/FTL speed modes
- collision/physics
- actual ship dimensions loaded from an asset

The immediate goal is to make local flight tuning meaningful in real units while keeping the existing renderer stable.
