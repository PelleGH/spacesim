# Hyperdrive v1

This pass adds the first system-scale travel operation to the modern SDL3/OpenGL + EnTT game.

## Controls

- `Tab` cycles available hyperdrive jump points.
- `J` engages hyperdrive to the selected jump point.

The selected target is shown in the window title and printed to the console when selection changes.

## Current prototype target

Earth is the only jump point. It is registered by adding a `JumpPointComponent` to the existing global planet entity.

The first arrival is intentionally **120 km above the physical surface**. This keeps the ship just above the atmosphere while the game still uses the renderer's distant/orbital planet representation. Once the real orbit-to-surface render transition exists, the target altitude can be lowered without changing the hyperdrive architecture.

## Architecture

### `JumpPointComponent`

A global-space entity becomes selectable by adding:

- a display name
- an arrival distance from the target's global center

This means future moons, stations, satellites, jump gates, or other POIs can join the target list without special-casing the hyperdrive system.

### Player hyperdrive state

The player ship has:

- `HyperdriveComponent` — currently selected target
- `HyperdriveControlComponent` — one-frame cycle/engage requests

`PlayerControlSystem` translates SDL input into those requests. `HyperdriveSystem` does not depend on SDL.

### Jump operation

A successful jump:

1. resolves the selected target's `GlobalPositionComponent`
2. chooses an arrival point on the same radial side of the target as the player's current approach
3. changes `GameWorld::localBubbleOriginMeters` to the arrival global position
4. returns the player ship to local `(0,0,0)`
5. points the ship toward the target
6. clears translational and angular velocity
7. unloads prototype entities marked `LocalBubbleTransientComponent`
8. resets the chase camera so it snaps to the new bubble rather than interpolating across system-scale distance

The renderer is not teleported directly. It simply sees the new global/local relationship on the next extraction pass.

## Why this uses the bubble origin

Hyperdrive is a large-scale world operation, not local ship thrust. Moving the active bubble keeps local flight coordinates near zero and avoids feeding astronomical positions into local simulation or float rendering.

## Next likely additions

- hyperdrive charge/engage/disengage state and audiovisual sequence
- jump target HUD instead of debug title/console only
- target eligibility/range/rules
- multiple planets/moons/stations
- destination-specific arrival vectors/zones
- orbit-to-surface render transition so arrivals can move much closer than 120 km
