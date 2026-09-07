# Vertical-Slice Roadmap

## Wave 0 — Architecture cleanup

- **TU-001:** canonical weapon ownership.
- Engine association/config cleanup.
- Input/config foundation.
- Basic automated compile/test path.

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
