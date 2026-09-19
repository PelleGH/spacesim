# Current State

This file is intentionally conservative.

The project-description PDF defines architecture and roadmap goals, but it does **not** establish the current implementation status of the live repository. Therefore, this file should be updated by inspecting the actual codebase rather than guessing.

## Status

### Confirmed from project documentation

- The intended initial framework is SDL3 + OpenGL + Dear ImGui.
- The project is planned as a custom engine with a staged roadmap.
- The architectural target includes a global layer, local gameplay bubbles, POIs, distant rendering, and later scale/precision systems.
- Full procedural planets, seamless landing, terrain streaming, atmospheric flight, and multiplayer are not early-stage requirements.

### Needs codebase inspection

Update these after opening/building the current repository:

- [ ] Current build system
- [ ] Current compiler/platform targets
- [ ] SDL3 initialization status
- [ ] OpenGL context status
- [ ] ImGui integration status
- [ ] Renderer abstraction status
- [ ] Shader system status
- [ ] Texture system status
- [ ] Mesh/model loading status
- [ ] Camera implementation status
- [ ] Debug drawing status
- [ ] Ship flight model status
- [ ] Collision/physics status
- [ ] Starfield status
- [ ] Distant sun/planet/moon rendering status
- [ ] Targeting status
- [ ] Station/asteroid gameplay status
- [ ] Save/load status
- [ ] Known performance issues
- [ ] Known rendering issues
- [ ] Current highest completed roadmap era

## Active milestone

**Unknown from the supplied architecture document.**

Replace this line after inspecting the live project.

## Current blockers

**Unknown from the supplied architecture document.**

List concrete build, rendering, gameplay, or architecture blockers here after inspection.

## Next recommended task

Do not infer this from the roadmap alone.

Choose the next task after comparing:

1. the actual codebase state,
2. the current highest working milestone,
3. the earliest unmet roadmap win condition.
