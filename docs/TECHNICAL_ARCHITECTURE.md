# The Unit — Technical Architecture

## Engine + Platform

- Engine: Unreal Engine 5
- Primary target (prototype): PC
- Networking model: authoritative server for co-op sessions

## High-Level Architecture

- `Source/TheUnit/` for gameplay C++ module(s).
- `Content/TheUnit/` for Blueprints, assets, maps, data.
- Blueprints for rapid iteration; C++ for core gameplay framework and performance-critical systems.

## Current C++ Class Skeleton (Phase 1A)

### Core Module
- `TheUnit.Build.cs` — Runtime module definition and engine dependency list.
- `TheUnit.cpp` / `TheUnit.h` — Primary game module bootstrap.

### Gameplay Framework Classes
- `UTU_GameInstance` — owns global runtime/session lifecycle and future persistent progression hooks.
- `ATU_GameMode` — owns authoritative mission rules and match flow.
- `ATU_GameState` — owns replicated mission-wide runtime state for clients.
- `ATU_PlayerController` — owns player input routing and client-side tactical command hooks.
- `ATU_PlayerState` — owns replicated per-player mission status and rewards metadata.

### Gameplay Actor Bases
- `ATU_OperatorCharacter` — owns operator avatar/pawn-level gameplay integration points.
  - Phase 1B responsibilities:
    - first-person camera + owner-only visible arms mesh placeholder components;
    - baseline movement/look input routing (`MoveForward`, `MoveRight`, `LookUp`, `Turn`);
    - tactical movement state toggles (`Sprint`, `Crouch`, `ADS`, `LeanLeft`, `LeanRight`) and speed selection;
    - interaction input entry point (`Interact`) for later gameplay system integration.
- `ATU_WeaponBase` — owns shared weapon actor contract and replication-safe weapon state shape.
  - Phase 1C responsibilities:
    - weapon identity + baseline tuning surface (`WeaponDisplayName`, fire rate, damage);
    - ammo state skeleton (`MagazineCapacity`, current magazine ammo, reserve ammo);
    - basic fire and reload state gates (`bCanFire`, `bIsReloading`) exposed to Blueprint;
    - compile-safe core functions (`Fire`, `CanFire`, `StartReload`, `FinishReload`, `AddReserveAmmo`) without projectile/recoil/audio/network behavior yet.
- `ATU_InteractableBase` — owns common world interaction contract and authority checks.
- `ATU_ObjectiveBase` — owns mission objective lifecycle and completion state surface.
- `ATU_ExtractionZone` — owns extraction area validation and extraction completion trigger points.
- `ATU_CalloutManager` — owns team callout event routing and future tactical callout coordination.

## Blueprint-Driven in Early Iteration

During early vertical-slice development, the following remain primarily Blueprint-driven:

- Map scripting and encounter composition.
- Objective presentation and in-level mission sequencing glue.
- Temporary UI/HUD widgets and mission feedback.
- Audio/VFX timing, cues, and presentation polish.
- Animation state machine tuning and montage sequencing.

C++ will provide stable gameplay foundations while Blueprint content continues to iterate rapidly.

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
