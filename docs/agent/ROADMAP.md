# Vertical-Slice Roadmap

## Wave 0 — Architecture cleanup

- **TU-001:** canonical weapon ownership.
- **TU-002:** UE 5.7 generator color compile prerequisite.
- **TU-003:** align project engine association with UE 5.7.
- **TU-004:** committed legacy input/config foundation.
- Basic automated compile/test path.

## Wave 0.5 — Modular Weapon Foundation

Complete this before player weapon equip/firing integration.

- Data-driven weapon platform definitions.
- Reusable weapon-part definitions and explicit part slots/interfaces.
- Compatibility metadata/tags that determine valid installations without per-build subclasses.
- Persistent/runtime weapon-instance composition from installed parts.
- Dedicated fire-control/trigger modules that determine supported fire modes and trigger-related gameplay characteristics.
- Derived weapon configuration/stat aggregation from the installed build.
- Focused compatibility, invalid-build, and configuration regression tests.

The system must support deep customization while remaining an abstract gameplay/data model rather than a physical-construction simulation.

## Wave 1 — Playable Combat

Operator health integration; weapon equip/ownership; fire, reload, and fire-mode input; timed firing; basic hitscan; regional damage and death; real Kill House PlayerStart.

## Wave 2 — Enemy Encounter

Enemy character and controller; perception; patrol and engagement; enemy firing/death; Kill House spawn-marker conversion.

## Wave 3 — Raid Loop

Interaction; Secure Intel objective; real extraction zone; GameMode raid lifecycle and success/failure; minimal HUD.

## Wave 4 — Loot / Persistence

ItemDefinition and ItemInstance; loot, equipment, raid inventory, stash, raid deltas, and local persistence.

## Wave 5 — Ready Room

Ready Room, loadout, deployment, post-raid results, and rebuild-kit loop.

## Wave 6 — Co-op

Authoritative combat; replicated health and AI outcomes; replicated objectives, extraction state, and raid results; two-player verification.

## Wave 7 — Polish

Weapon feel, recoil, animation, audio, VFX, lighting, AI tuning, map flow, and optimization.

Finish and validate each wave as a thin end-to-end increment. Stadium work remains deferred until the Kill House loop is healthy.
