# The Unit Tactical PvE Extraction Design Report

## Executive Summary

The recommended starting path for **The Unit** is to build the first playable version in **Unreal Engine 5** with a **Blueprint-first, C++-for-core-systems** approach.

This recommendation is based on practical fit:
- Unreal is free for game development until a title exceeds USD $1M gross revenue.
- Unreal provides strong first-party workflows for modular characters, Blueprints, C++, replication, and asset management.
- Unreal integrates naturally with Fab and Epic Online Services.

**Godot** remains the strongest open-source alternative (MIT license, lightweight tooling, rapid scripting), but will typically require more custom pipeline engineering for this genre’s systems complexity.

The Unit’s intended identity should blend:
- **Operator-style embodiment** (body presence, gear expression, tactile handling), and
- **Tarkov-style persistence** (stash pressure, extraction risk, long-term progression).

### Legal/Branding Guardrails

The Unit should emulate **system goals**, not **specific expression**:
- Do not copy visual identity, icon language, item-card styling, faction naming, UI composition, or branded insignia from reference games.
- Run trademark clearance for the title **"The Unit"** before public marketing.

---

## Core Design Pillars

| Pillar | Direction for The Unit |
|---|---|
| Operator identity | Full-body presentation, patch/insignia expression, ready-room tone, personalized voice profiles |
| Meta progression | Compact but durable stash/preset/reward loop with meaningful long-term build trees |
| Weapon building | Tarkov-style slot logic with intentionally capped platform breadth for readability |
| Ballistics/damage | Ammo-led lethality, armor-area survivability, weapon-led handling and delivery |
| PvE loop | Briefing → infil → objective tension → extraction → stash triage → kit rebuild |

---

## Callouts and Interface Direction

Use three communication layers:
1. **Plain tactical speech** (instant gameplay clarity)
2. **Formal radio prowords** (discipline/immersion)
3. **Personality delivery layer** (faction/fatigue/suppression flavor)

Operational VO rule: auto-fire only lines that reduce uncertainty; route everything else through manual/semi-manual inputs (quick wheel/ping).

---

## Systems Architecture

Adopt a strict split between immutable definition data and mutable instance data:

- `ItemDefinition`: static design truth
- `ItemInstance`: runtime/player-owned state
- `AmmoDefinition`: ballistic identity and armor interaction
- `WeaponPlatformDefinition`: socket and action constraints
- `OperatorProfile`: progression/unlocks/ownership
- `Stash`: spatial inventory + presets + raid deltas

### Key Principle

Treat attachments, ammo, armor plates, patches, containers, and medical as items with shared persistence and inspection patterns, plus specialized behavior components where needed.

---

## Engine and Toolchain Decision

### Recommended Build Path

- **Engine**: Unreal Engine 5
- **Gameplay scripting/prototyping**: Blueprints
- **Integrity/performance systems**: C++

### Practical Free/Low-Cost Pipeline

- Asset sourcing: Fab, Mixamo, Poly Haven, Sonniss, Freesound
- Authoring: Blender, Krita, Inkscape, Audacity
- Local persistence: SQLite
- Online services later: EOS + Supabase or EOS + PlayFab

---

## Networking and Persistence Model

Use mixed authority modes:

- **Solo dev/debug**: local authority acceptable
- **Live solo progression**: backend-validated raid result commit
- **Co-op raids**: authoritative host/dedicated authority for AI/loot/hits/objectives/extraction

Persistence should commit **raid deltas** atomically (not whole-profile overwrites):
- item gains/losses
- durability/ammo consumption
- wounds/reputation/task progress
- preset updates

---

## MVP Scope (First Meaningful Slice)

- 1 medium map, 2 extracts
- 1 faction, 2 voice profiles
- 4 wearable layers, 3 patch anchors
- 6 weapons, 12 attachment families, 8 ammo types
- 2 armor carriers, 3 plate classes, 1 backpack tier
- 6 AI archetypes
- 3 mission types (secure intel, destroy cache, escort package)
- Solo + 2-player co-op
- Persistent stash + presets + end-of-raid delta commits

---

## Five-Phase Production Plan

1. **Combat Graybox**
2. **Kit Laboratory**
3. **Raid Persistence Slice**
4. **Alpha Economy**
5. **Content/Balance Polish**

---

## GitHub-First Development Note

Since the project is being built in Unreal Engine while code is stored on GitHub, use this baseline source-control policy from day one:

1. Commit C++ source, configs, data definitions, and design docs frequently.
2. Use Git LFS for large binary assets (`.uasset`, `.umap`, high-res source files).
3. Keep gameplay systems data-driven to reduce merge conflicts.
4. Use branch-per-feature with short-lived pull requests.
5. Enforce naming conventions for assets and folders early.
6. Maintain lightweight changelog notes per feature PR.

Suggested Unreal `.gitignore` + `.gitattributes` setup should be added as soon as the UE project skeleton is created.

---

## Final Recommendation

Build The Unit in **Unreal Engine 5 first**.
Use **Blueprints for fast iteration** and **C++ for authoritative/core systems**, keep the initial content scope tightly constrained, and optimize for a readable, inspectable tactical experience with persistent consequences.
