# Architectural Decisions

Source: `spacesim project description (3).pdf`

This file records decisions already established by the project description. It should not be used to invent new design choices.

## D-001 — Keep the project playable during development

**Decision:** Prefer small, working prototypes and frequent playable states.

**Reasoning in source:** The project should remain easy to iterate on and should validate gameplay before adding large systems.

---

## D-002 — Use third-party libraries for general infrastructure

**Decision:** Use permissive libraries for common engine infrastructure and reserve custom engineering effort for space-sim-specific problems.

**Planned libraries include:** SDL3, OpenGL, Dear ImGui, Jolt Physics, miniaudio, stb, Assimp, GLM, nlohmann/json, and spdlog.

---

## D-003 — Start with SDL3 + OpenGL + ImGui

**Decision:** Phase 1 uses SDL3 + OpenGL + Dear ImGui.

**Constraint:** Keep rendering architecture flexible enough that the backend can be replaced later.

---

## D-004 — Separate global scale from local gameplay simulation

**Decision:** Use a global position layer for large-scale placement and a local gameplay bubble for detailed simulation.

**Implication:** Do not require astronomical/global coordinates to directly drive all local physics.

---

## D-005 — Keep POIs globally persistent but locally activatable

**Decision:** Stations, asteroid fields, planets, moons, wrecks, and other POIs exist globally and activate richer local content when relevant.

**Implication:** The player should not be trapped in hard instances.

---

## D-006 — Allow deep-space gameplay away from authored POIs

**Decision:** The player may drop out of travel mode anywhere.

**Implication:** Deep-space local bubbles may be empty or procedurally populated with encounters.

---

## D-007 — Keep early orbital motion simple

**Decision:** Early orbital movement affects global/map/render state rather than local ship physics.

**Implication:** Local gameplay around an orbiting station should remain stable unless orbital dynamics are explicitly required for gameplay.

---

## D-008 — Treat sectors/floating origin as precision tools

**Decision:** Large-distance precision may use sectors and/or floating origin.

**Constraint:** Do not model space as a dense Minecraft-style chunk grid.

---

## D-009 — Render space in layers

**Decision:** Use deep background, procedural stars, distant bodies, and local objects as separate visual layers.

**Implication:** A single flat skybox should not be the only representation of space.

---

## D-010 — Use fake local render distance for distant bodies

**Decision:** Planets, moons, and similar objects should not be rendered at true physical distance.

**Implication:** Global coordinates determine direction/apparent scale/travel meaning, while the renderer uses a safe fake distance and matching visual scale.

---

## D-011 — Stage planet rendering

**Decision:** Planet rendering develops in three stages:

1. distant body
2. orbit body
3. surface body

**Constraint:** Surface-body terrain/landing technology is not part of the early prototype.

---

## D-012 — Defer expensive dream features

**Decision:** Seamless planet landing, procedural planets, atmospheric flight, multiplayer, multicrew, and a large persistent universe are late experimental features.

**Implication:** They are not requirements for the project to be successful.

---

## D-013 — Validate flight before building the larger game

**Decision:** The flight toy must become enjoyable before the project grows into a wider simulation.

**Era 1 win condition:** The player can fly around for 10 minutes and enjoy the feel.

---

## D-014 — Prove one complete loop before expanding the simulation

**Decision:** The small-space sandbox should establish one complete gameplay loop before adding broader world simulation.

**Era 2 win condition:** Launch, travel somewhere, perform one activity, return, and receive a result/reward.
