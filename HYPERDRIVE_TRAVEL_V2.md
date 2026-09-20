# Hyperdrive Travel v2

The first jump implementation relocated the local bubble instantly. This version makes
hyperdrive a real system-scale travel state that advances through the double-precision
global coordinate layer.

## Controls

- `Tab` cycles selectable `JumpPointComponent` targets.
- `J` begins a jump to the selected target.

## State machine

`Idle -> Aligning -> Traveling -> Idle`

### Aligning

The normal ship movement system is suspended. `HyperdriveSystem` turns the ship toward
the resolved arrival point at a bounded angular rate. Travel does not begin until the ship is
within the alignment tolerance.

### Traveling

The ship remains at local gameplay origin for precision while
`GameWorld::localBubbleOriginMeters` advances through global space every fixed simulation
tick. This is real global-coordinate motion, not a timed visual effect or end-of-jump
teleport.

Travel uses a simple acceleration / braking profile:

- 2,000 km/s^2 prototype hyperdrive acceleration
- 6,000 km/s prototype maximum travel speed
- braking speed is derived from remaining distance so the ship can stop at the arrival point

The window title displays current hyperdrive speed and remaining distance during travel.

## Planet arrival

Earth still exits 120 km above the physical surface. Planet arrival points are chosen on the
illuminated hemisphere using the primary star direction. Rather than always targeting the
exact sub-solar point, the system selects the closest approach direction that keeps the sun
comfortably above the arrival horizon. This avoids unnecessarily routing a straight jump
through the planet when starting from the opposite side.

## Local bubble handling

When a jump begins:

1. The bubble is rebased so the player ship becomes local `(0,0,0)` without changing its
   global position.
2. Temporary local prototype content is unloaded rather than being carried across the star
   system.
3. Local linear/angular flight velocity is cleared.
4. During hyperdrive the bubble origin itself moves through `glm::dvec3` global metres.

Normal `ShipMovementSystem` simulation ignores a ship while its hyperdrive state is active.
The chase camera remains a local presentation system and therefore follows the ship normally
through alignment and travel.

## Next likely additions

- hyperdrive spool/charge time and audio/visual effects
- HUD target marker and alignment indicator
- player-controlled abort / interdiction behavior
- minimum jump distance / mass-shadow rules
- generated jump points for moons, stations and other global POIs
