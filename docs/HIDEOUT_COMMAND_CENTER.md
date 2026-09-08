# Hideout-Style Special Operations Command Center

## Goal

The Unit's pre-mission headquarters should feel like a lived-in, improvised, secure special-operations hideout rather than a polished front-end menu or pristine office building.

The reference point is the *idea* of Escape from Tarkov's Hideout: one physical space that communicates preparation, ownership, maintenance, storage and progression. The Unit should not copy Tarkov room layouts, props, UI, branding or exact visual composition.

## Core player loop

1. Load into the headquarters as the operator.
2. Walk through the physical base.
3. Visit the Armory to select primary/secondary weapons.
4. Use weapon benches to configure attachments.
5. Walk into the Cage to select armor, uniforms, helmet, carrier, belt, pack and other equipment.
6. Use the test range to verify the selected weapon setup.
7. Gather in the briefing/planning area.
8. Pull the chest-mounted MX50 to review mission package, map, intel, team, feeds and loadout.
9. Authorize/deploy into the operation.

No ordinary main-menu flow should replace these physical interactions during normal gameplay.

## Visual language

Target a believable working facility:

- reinforced concrete / painted block / patched wall surfaces
- exposed conduit, piping and cable runs
- utility panels and breaker cabinets
- fluorescent strips mixed with localized work lights
- steel cage storage and open shelving
- stacked transit cases and plastic bins
- tool benches and parts bins
- lockers and ready racks
- medical cot / supply cabinet
- comms/radio rack
- mission boards and planning surfaces
- generator / battery backup area
- worn floors, scuffed paint and accumulated practical clutter

Avoid:

- glossy sci-fi architecture
- luxury PMC clubhouse styling
- giant holograms
- sterile showroom weapon walls
- perfect symmetry everywhere
- decorative props that imply functionality the player can never use

## Existing functional rooms remain

The hideout layer augments, rather than replaces:

- Armory
- Weapon customization benches
- Cage / equipment room
- Uniform / gear bench
- Test-fire range
- Briefing / planning space
- Chest-mounted MX50 mission interface

## Progression modules

`UTUHideoutProgressionComponent` tracks eight independent module levels from 0 to 3:

- Power
- Communications
- Medical
- Storage
- Armory Support
- Gear Maintenance
- Planning
- Range Support

Level meanings:

- **0** — unbuilt / unavailable
- **1** — basic operational capability
- **2** — developed capability
- **3** — fully developed capability

The component intentionally does not own money, crafting materials or unlock requirements. A later campaign/meta system should decide whether an upgrade is affordable and permitted, then call the hideout progression API.

## Graybox decorator

`ATU_HideoutCommandCenterDecorator` is an additive environment actor intended to share the transform of `ATU_CommandCenterGenerator`.

At BeginPlay it can automatically snap to the first command-center generator in the level. Its first graybox pass adds:

- generator and electrical controls
- exposed overhead conduit
- storage racks and transit cases
- operator staging bench / ready rack
- weapon/gear maintenance bench and parts bins
- expandable medical area
- communications rack and radio desk
- planning board and table support

These are placeholder primitives, not final environment art.

## Progression philosophy

Upgrades should make the base visibly change. A level increase should not be only a number in a menu.

Examples:

- Power 1: portable generator; Power 2: battery rack; Power 3: permanent control cabinet / improved lighting support.
- Medical 0: unopened cases; Medical 1: cot and cabinet; Medical 2+: expanded supplies and treatment capability.
- Gear Maintenance 1: basic bench; higher levels add dedicated cleaning/repair stations and storage.
- Communications levels add increasingly developed radio/network equipment.
- Storage levels add racks, cases and organized bulk storage.

Later gameplay benefits may include readiness, repair, intelligence, training or logistics advantages, but those should be designed separately from the visual module system.

## Acceptance criteria

1. The headquarters remains fully walkable.
2. Existing Armory/Cage/Briefing/Range station interactions remain unchanged.
3. The environment reads as inhabited and operational from first entry.
4. Utility systems have believable placement and do not obstruct traversal.
5. Module levels visibly change at least one area of the hideout.
6. Module state is data-driven and callable from Blueprint/campaign code.
7. Medical level 0 visibly reads as unfinished/unbuilt rather than silently disappearing.
8. The command-center generator remains the functional hub source of truth; the hideout decorator remains additive.
9. Final environment art preserves The Unit's own special-operations identity rather than copying another game's hideout.
10. UE 5.7 compile, construction-script behavior, PIE traversal and collision are validated before merge.

## Current validation status

Code and automation tests have been authored, but UE 5.7 compile, automation execution and PIE validation have not yet been run.
