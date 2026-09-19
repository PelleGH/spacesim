# Architecture

Source: `spacesim project description (3).pdf`

## 1. Engine foundation

The initial framework is based on SDL3 + OpenGL + Dear ImGui, with the renderer structured so graphics backends can be swapped later.

Planned/supporting libraries:

- SDL3 — window, input, events
- OpenGL — initial rendering API
- Dear ImGui — debug tools
- Jolt Physics — physics/collision
- miniaudio — audio
- stb — image/font helpers
- Assimp — model importing
- GLM — math
- nlohmann/json — config/data files
- spdlog — logging

## 2. Core principles

The project should:

- remain playable as often as possible
- grow through small working prototypes
- use permissive libraries for general engine infrastructure
- reserve custom systems for space-sim-specific needs
- choose simple working implementations before complex scalable ones
- postpone multiplayer, planets, interiors, and large-scale simulation until the core game feels good

## 3. Long-term world-scale architecture

The world should feel continuous without being fully simulated everywhere.

### 3.1 Global layer

The global layer represents the star system at large scale.

It stores positions for major objects such as:

- stars
- planets
- moons
- stations
- asteroid belts
- jump points
- signal sources
- other POIs

Its main responsibilities are:

- travel distance
- system-map location
- discovery/navigation
- distant-rendering decisions
- spawning rules
- persistence
- simple orbital motion

It should not be required to drive detailed local physics during early development.

### 3.2 Local gameplay bubble

Detailed gameplay runs in an active local bubble around the player or another relevant POI.

Responsibilities include:

- ship movement
- nearby rendering
- physics/collision
- AI
- weapons
- docking
- mining
- scanning
- local encounters

Anything outside the bubble is dormant, abstracted, or rendered only as a distant visual.

This allows dropping out of travel mode anywhere, including deep space.

### 3.3 Points of interest

POIs such as stations, asteroid fields, planets, moons, and wrecks exist globally.

When approached, a POI's richer local gameplay area becomes relevant and overlaps with the player's local bubble.

A station, for example, may define:

- global position
- local station layout
- docking zones
- traffic rules
- patrols
- collision geometry
- distant-render behavior

Leaving the area returns the POI to an abstract global object or distant visual.

### 3.4 Deep space

The player can exist anywhere in a star system, not only at authored locations.

Dropping from travel in deep space creates a sparse local bubble. It may contain nothing, or procedural encounters such as:

- signal sources
- wrecks
- pirates
- distress calls
- anomalies
- mineable rocks
- hidden caches

This supports interdiction, ambushes, exploration, hiding, camping, and emergent events away from POIs.

### 3.5 Orbits

Simple orbital rules may later move planets, moons, stations, and other POIs.

Initially, orbital motion should affect only:

- global position
- system-map location
- travel target location
- distant rendering
- visual/narrative believability

Local gameplay around an orbiting station should remain stable. Orbital velocity should not drive local ship physics unless gameplay explicitly needs it.

### 3.6 Coordinate sectors and floating origin

The project may later use sector coordinates and/or floating-origin techniques to avoid precision issues at large distances.

These are coordinate/precision tools, not dense world chunks.

Preferred model:

- global positions
- local active bubble
- POI interest zones
- procedural deep-space encounters
- distant rendering
- travel mode

## 4. Space background and distant-body rendering

Space should be rendered as layered visual systems rather than a single static skybox.

### 4.1 Rendering layers

1. Deep background
2. Procedural starfield
3. Distant bodies
4. Local gameplay objects

A typical full render order is:

1. Clear screen
2. Draw deep background
3. Draw procedural starfield
4. Draw distant bodies such as sun, planets, and moons
5. Draw local gameplay objects
6. Draw debug visuals
7. Draw UI

Sky/background elements should not participate in normal gameplay collision.

### 4.2 Deep background

The deep background can contain:

- subtle nebula haze
- faint color gradients
- galaxy bands
- other large-scale visual features

### 4.3 Procedural starfield

A star may contain:

- direction from camera
- brightness
- size
- color tint
- twinkle speed
- twinkle phase
- optional parallax amount

Stars should generally behave as if infinitely far away: they follow camera position but respond to camera rotation.

Bright stars may use small glow billboards; dim stars may use points or tiny quads.

Twinkle should remain subtle.

### 4.4 Parallax

Possible background depth layers:

- far stars with no parallax
- mid-depth stars/dust with tiny parallax
- faint nebula/dust with slightly stronger parallax

Parallax must remain subtle enough that space does not feel like a small dome.

### 4.5 Distant planets and moons

Distant bodies should not be placed at true physical distance in the local render scene.

Instead, store/derive:

- direction
- apparent angular size
- visual radius
- texture/material
- atmosphere color
- cloud-layer data
- lighting direction

The global layer determines direction, scale, travel distance, map position, and visibility. The renderer places the object at a safe fake distance and scales it to the desired apparent size.

This avoids precision problems while retaining the visual impression of huge distances.

### 4.6 Planet rendering stages

#### Stage 1 — Distant body

- simple textured sphere or billboard
- no collision
- no terrain
- no local gameplay

#### Stage 2 — Orbit body

- larger sphere
- improved texture detail
- lighting
- atmosphere shell
- cloud layer
- optional night-side color

#### Stage 3 — Surface body

Future terrain and landing technology.

This is explicitly not part of the early prototype.

### 4.7 Sun

The star/sun can be represented by:

- bright emissive sphere or billboard
- glow sprite
- main directional light source
- reference direction for planet lighting

The prototype does not require real scale or physical reachability.

## 5. Early rendering implementation target

Build:

- procedural starfield
- subtle star brightness variation
- a few brighter glowing stars
- one distant sun
- one large distant planet
- one smaller moon
- optional faint nebula/background haze

Do not build:

- full procedural planets
- seamless planet landing
- real astronomical-scale rendering
- complex orbital physics
- terrain streaming
- atmospheric flight

The purpose is to make the current flight prototype feel situated in real space without taking on full universe or planet technology.
