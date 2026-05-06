# The Unit — Roadmap

## Mini-Phase Status (Current)

- Phase 0 complete
- Phase 1A complete
- Phase 1B complete
- Phase 1C ammo skeleton complete
- Phase 1C.1 fire mode foundation complete
- Phase 1D.1 automated kill house generator
- Phase 1D.2 stadium generator

## Phase 1 — Vertical Slice

- Deliver one end-to-end playable extraction mission.
- Validate tactical co-op loop with internal playtests.
- Establish baseline performance and networking behavior.

## Phase 1D.1 — Automated Kill House Generator

- Implement `ATU_KillhouseGenerator` to auto-build a two-story CQB kill house in-editor.
- Expose layout controls for dimensions, room counts, hallway/stair widths, roof, and debug labels.
- Generate tactical placeholders for spawn, objective, extraction, enemy spawn, and patrol points.

## Phase 1D.2 — Stadium Generator

- Implement `ATU_StadiumGenerator` for drag-and-drop in-editor asymmetrical stadium generation.
- Use public field dimensions to shape tactical graybox outfield walls and deep-center triangle.
- Provide gameplay marker generation and debug callout labels for encounter setup.

## Phase 2 — Core Expansion

- Additional objective types and enemy variants.
- Expanded player kit options (weapons/utilities).
- Early persistence and progression scaffolding.

## Phase 3 — Production Foundations

- Content pipeline hardening and conventions.
- Improved UI/UX and readability polish.
- Test automation, build workflows, and stability milestones.

## Phase 4 — Pre-Release Iteration

- Balance passes informed by playtest telemetry.
- Content breadth expansion (maps, missions, factions).
- Performance optimization and bug triage.

## Milestone Governance

Each phase exit should require:
- Clear acceptance criteria.
- Playtest results + issue log review.
- Decision checkpoint on scope, schedule, and risks.
