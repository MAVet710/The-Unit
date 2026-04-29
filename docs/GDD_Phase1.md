# The Unit — Game Design Document (Phase 1)

## Objective

Define and ship a **playable vertical slice** that validates the tactical co-op extraction loop.

## Phase 1 Scope

### Player Experience Goals

- 1–4 player co-op insertion into a contained map.
- PvE combat against at least one enemy archetype set.
- Primary objective completion + optional side objective.
- Extraction under pressure with success/failure states.

### Core Systems (Phase 1)

- Movement and camera baseline.
- Weapon handling for one primary weapon class.
- Health, downed/revive, and basic healing flow.
- Enemy perception and combat behavior (baseline AI).
- Match flow: deploy → engage → complete objective → extract.
- Basic loot and reward placeholder logic.

### Content Targets

- 1 test map with 2–3 combat spaces and one extraction zone.
- 1 player operator archetype (placeholder art acceptable).
- 2–3 enemy types (e.g., rifleman, flanker, heavy).
- 1–2 objective templates (e.g., data retrieval, holdout).

### UX / UI Targets

- Health, ammo, objective status, extraction timer.
- Team status panel (alive/downed/extracted).
- End-of-match summary (success/failure + rewards).

## Out of Scope (Phase 1)

- Full meta-progression economy.
- Extensive narrative/cutscenes.
- Large-scale map variety.
- Final animation polish and production art fidelity.

## Validation Criteria

Phase 1 is complete when internal playtests confirm:

- The mission loop is understandable and repeatable.
- Tactical team coordination is materially rewarded.
- Failure/success outcomes feel fair and legible.
- Technical performance remains stable in target test conditions.
