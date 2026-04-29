# The Unit

A tactical co-op PvE extraction shooter prototype built in **Unreal Engine 5**.

## Project Status

This repository is currently in **Phase 0: Foundation**.

Phase 0 goals:
- Define project structure and documentation baseline.
- Establish source control conventions for UE5 assets.
- Prepare folders for gameplay, content, and systems work.

## Core Vision (Summary)

- Squad-based tactical PvE with meaningful risk/reward.
- Session-based extraction loop with persistent progression.
- Strong emphasis on communication, positioning, and encounter planning.

For full design context, see:
- `THE_UNIT_TACTICAL_PVE_EXTRACTION_DESIGN_REPORT.md`

## Repository Layout

- `Source/TheUnit/` — C++ gameplay module root (placeholder in Phase 0).
- `Content/TheUnit/` — UE5 content root for project assets.
- `docs/` — planning and technical documentation.

## Phase 0 Outputs

- Unreal-friendly `.gitignore`
- Git LFS tracking rules in `.gitattributes`
- Initial planning documents:
  - `docs/GDD_Phase1.md`
  - `docs/TECHNICAL_ARCHITECTURE.md`
  - `docs/ROADMAP.md`

## Tooling & Version Control

This repo expects **Git LFS** for large binary assets.

Suggested setup:

```bash
git lfs install
git lfs pull
```

## Next Milestone

Move into **Phase 1: Playable Vertical Slice** with a focused test map, one operator archetype, one enemy faction, baseline combat loop, and extraction flow.
