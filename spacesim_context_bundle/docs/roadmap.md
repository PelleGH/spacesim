# Roadmap

Source: `spacesim project description (3).pdf`

## Era 0 — Engine Foundation

### Goal

Create the minimum engine base needed to render, update, debug, and iterate.

### Build

- application entry point
- SDL3 window/input/events
- OpenGL context
- ImGui integration
- logging
- basic file loading
- basic time/update loop
- basic renderer abstraction
- shader loading
- texture loading
- simple mesh rendering
- camera
- debug drawing

### Win condition

The engine opens a window, renders a simple 3D scene, shows an ImGui debug panel, and can be cleanly built/run.

---

## Era 1 — The Flight Toy

### Goal

Make flying fun before building a larger game around it.

### Build

- one controllable ship
- 6DOF movement
- thrust, rotation, boost, damping, and flight-assist tuning
- camera modes
- skybox/starfield
- a few test objects
- simple collision
- targeting test object
- debug UI for tuning flight values

### Do not build

- planets
- networking
- economy
- interiors
- missions
- large world simulation

### Win condition

You can fly around for 10 minutes and enjoy the feel.

---

## Era 2 — The Small-Space Sandbox

### Goal

Create a tiny playable area with one complete gameplay loop.

### Build

- asteroids
- one station
- simple docking
- basic weapons, scanning, or mining
- simple mission objective
- radar/targeting
- basic inventory or cargo
- save/load
- simple UI for objective/status

### Win condition

Launch, fly somewhere, do one activity, return, and receive a reward/result.

---

## Era 3 — The Sim Layer

### Goal

Make the world feel alive and give the player reasons to fly.

### Build

- simple AI ships
- basic traffic around stations
- factions
- cargo types
- simple economy variables
- contracts
- reputation
- basic spawn/despawn rules
- simple comms/messages

### Win condition

The player can choose between different activities, and the world appears to continue existing beyond the player.

---

## Era 4 — The Scale Layer

### Goal

Make space feel large without breaking physics, rendering, or precision.

### Build

- floating origin or sector-based coordinates
- orbital bodies as distant objects
- travel modes
- system map
- procedural placement
- streaming sectors
- location discovery
- long-range navigation
- persistence for visited/generated areas

### Win condition

You can travel between far-away locations without the engine falling apart.

---

## Era 5 — Immersion Features

### Goal

Add deeper physicality and player attachment after the core space game works.

### Possible features

- limited ship interiors
- walkable hangar/station interior
- better damage model
- component-based ship systems
- repair/refuel/rearm systems
- EVA prototype
- improved cockpit UI
- crew/NPC passengers

### Win condition

Ships and stations feel more physical and personal, without requiring the whole universe to be seamless.

---

## Era 6 — Dream / Experimental Features

### Goal

Explore the hardest long-term features only after the core game is stable.

### Possible features

- seamless planet landing
- procedural planets
- multicrew
- multiplayer
- large persistent universe
- complex damage simulation
- atmospheric flight
- ground vehicles
- base building

### Win condition

These are experiments, not requirements for the project to be successful.
