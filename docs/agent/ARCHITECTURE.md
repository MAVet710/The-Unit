# Architecture

## Product boundary

The current target is one narrow vertical slice: Ready Room -> Loadout -> Kill House -> Enemy Engagement -> Objective -> Loot -> Extraction -> Raid Result -> Persistent Stash. New work must advance this loop before broadening scope. The Kill House procedural graybox is the primary environment; Stadium work is deferred.

The project is an Unreal Engine 5 C++ project with runtime module `TheUnit`, targeting PC and an eventual authoritative co-op server. Targets use UE 5.7 build/include settings, but `TheUnit.uproject` still declares Engine Association 5.0. Treat this as a known configuration issue, not a reason to redesign gameplay.

## System ownership

- **Lead / Architect:** architecture, cross-system interfaces, sequencing, decomposition, and integration decisions.
- **Combat:** weapon actor, firing, ammunition, reload, fire modes, ballistics/damage delivery, and weapon handling.
- **Character:** operator, movement, health integration, interaction entry points, and character state.
- **AI:** enemies, controllers, perception, patrol, behavior, and tactical decisions.
- **Raid:** authoritative GameMode flow, objectives, extraction, raid state, and results.
- **Inventory:** item instances, ownership, loot, equipment, containers, stash, raid deltas, and persistence-facing inventory state.
- **UI:** HUD, Ready Room, loadout, inventory, and raid results.
- **World:** Kill House generator, placement hooks, spawn definitions, and environment support.
- **QA:** compilation, targeted automation, regressions, and diff-focused review.

One agent primarily owns each subsystem. Cross-subsystem changes require explicit declaration and Lead review of the contract boundary.

## Runtime boundaries

`ATU_GameMode`/Raid is the intended server-authoritative owner of raid lifecycle. `ATU_GameState` will expose replicated raid state; neither inventory nor UI determines outcomes. Character owns player-character state and supplies clean combat and interaction entry points. Reusable health belongs in `UTUHealthComponent` for players and eligible AI.

Combat must converge on `ATU_WeaponBase` as the single external runtime weapon API. A subordinate component may encapsulate ammunition mechanics, but it must not compete as a second weapon implementation. Shared definitions in `TheUnitTypes.h` remain canonical.

Inventory owns item identity and ownership. Raid consumes inventory-facing operations and eventually commits explicit raid deltas, rather than overwriting an entire profile. UI presents state and sends intent; it never owns authoritative rules.

World generators create geometry and gameplay placement information. They do not control raid state, spawn policy, objectives, or extraction outcomes. Marker components currently emitted by the Kill House are placeholders until owning gameplay systems consume them.

## Evolution rules

Prefer narrow interfaces and incremental integration over speculative abstractions. Local gameplay may precede networking, but every functional system must have an explicit path to server authority, validation, and replication. Do not duplicate a system because its implementation was not discovered; search for the relevant symbol first. Add Unreal module dependencies only when an implementation requires them.
