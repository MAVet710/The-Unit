# The Unit — Technical Architecture (Initial)

## Engine + Platform

- Engine: Unreal Engine 5
- Primary target (prototype): PC
- Networking model: authoritative server for co-op sessions

## High-Level Architecture

- `Source/TheUnit/` for gameplay C++ module(s).
- `Content/TheUnit/` for Blueprints, assets, maps, data.
- Blueprints for rapid iteration; C++ for core gameplay framework and performance-critical systems.

## Proposed System Layers

1. **Core Framework**
   - GameInstance, GameMode, GameState, PlayerState
   - Session flow and match state transitions
2. **Gameplay Systems**
   - Combat, damage, status effects, interaction
   - Objective and extraction systems
3. **AI Systems**
   - Perception, behavior trees, encounter scripting
4. **Data Layer**
   - DataAssets/DataTables for tunables and content definitions
5. **UI Layer**
   - HUD, team panel, objective tracking, end-match summary

## Networking Considerations

- Server-authoritative combat and objective outcomes.
- Replicated essential player/enemy states.
- Minimize replicated frequency for non-critical cosmetic data.
- Use prediction/interpolation where needed for responsiveness.

## Asset and Source Control Strategy

- Git for source + config + docs.
- Git LFS for binary content:
  - `.uasset`, `.umap`, `.fbx`, `.wav`, `.png`, `.tga`, `.blend`
- Standard UE5 transient directories ignored (`Intermediate`, `Saved`, etc.).

## Build and Environment Baseline

- Developer environments target UE5-compatible toolchains.
- CI/CD deferred to later phase; initial focus is local iteration stability.

## Risks (Early)

- Scope creep in systems before loop validation.
- Replication complexity if architecture boundaries are unclear.
- Content pipeline bottlenecks if naming/folder conventions are not enforced early.
