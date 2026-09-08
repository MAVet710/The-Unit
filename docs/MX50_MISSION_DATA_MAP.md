# MX50 Mission Packages, Tactical Map and Video Sources

## Purpose

This layer turns the operator-worn MX50 from a static briefing display into a data-driven mission tool. It is stacked above the persistent tablet work in PR #23.

The design rule is simple: **the tablet only shows mission information that has actually been authored or supplied by gameplay systems.** It must not invent floor plans, threat locations, team positions or live video sources.

## Mission package asset

`UTUMissionPackageData` is an Unreal `UDataAsset` that can be authored per operation.

It contains:

- mission id
- mission title
- objective
- AO / area label
- threat summary
- team summary
- deployment authorization
- tactical map markers
- planned route points
- registered video sources

A physical `ATU_CommandCenterStation` in the Briefing Room may reference a mission package. Pressing `F` at that station applies the full package to the same persistent MX50 carried by the operator.

If no mission package is assigned, the station keeps the older mission-id/title fallback behavior.

## Tactical map model

Map coordinates use normalized `0..1` coordinates rather than hard-coded world dimensions. This allows the same data to overlay a later floor-plan texture, satellite image, blueprint, generated map or mission-specific render without rewriting the marker system.

Marker types:

- Entry
- Objective
- Threat
- Rally
- Friendly
- Extraction
- Observation

Each marker has:

- stable marker id
- display label
- marker type
- normalized map position
- floor index
- details text
- visibility state

Runtime marker updates use upsert semantics so mission gameplay can update known threats, squad markers, rally points or objectives without rebuilding the full package.

The current native UI renders a clickable 12x8 tactical grid. That is a functional prototype, not final map art. When a proper Kill House floor plan is available, the same marker coordinates should be drawn over that image.

Selection automatically recovers if the selected marker is hidden or removed.

## Planned route

Mission packages may contain normalized route points. The component clamps route coordinates to the map boundary.

The current UI reports the route-point count. Production presentation should draw connected route segments over the map and allow authorized squad planning edits.

## Video source registry

The MX50 maintains a registry of video feeds independent of the separate FPV branch.

Supported source categories:

- FPV Drone
- Body Camera
- Fixed Camera
- Observation Feed

Each source has:

- stable feed id
- display name
- source type
- status text
- availability state

Only available feeds can be selected. If the selected feed drops, the MX50 automatically falls back to another available source. Overall FPV availability remains true while any registered FPV source is still available.

The current UI provides feed selection and status. It deliberately does **not** fake a video image. The live viewport should be connected later when a provider supplies a render target or equivalent video surface.

## FPV integration contract

PR #17 remains the source of truth for the actual FPV drone simulation/presentation work.

After that work is compiled and integrated, the bridge should:

1. register an `FPVDrone` MX50 feed when a controllable/observable drone feed exists;
2. update availability and link-status text as the drone signal changes;
3. provide the actual render target/video surface to the MX50 UI;
4. remove or mark the feed unavailable when the drone is destroyed, recovered or out of service;
5. preserve the MX50 feed-selection state without making the tablet own drone physics.

The tablet must not directly depend on the drone solver.

## Command-center flow

Target flow:

1. operator enters the Briefing Room;
2. physical briefing station references a `UTUMissionPackageData` asset;
3. operator presses `F`;
4. the chest-mounted MX50 is raised;
5. the full mission package is loaded;
6. Mission / Map / Intel / Team / Drone / Loadout pages use that persistent state;
7. operator stows the tablet and retains the selected mission/page;
8. operator may raise the same tablet again in the field with `T`.

## Automation

`TheUnit.MX50.MissionPackageMapVideo` now covers:

- package application
- map-coordinate clamping
- route-coordinate clamping
- marker selection
- marker removal
- hidden-marker selection recovery
- runtime marker upsert
- available-feed auto-selection
- offline-feed rejection
- multiple FPV-source availability
- selected-feed failover
- feed removal

Existing MX50 state/page and controller-wiring automation remains in place.

## Validation gate

Before this PR leaves draft:

1. UE 5.7 UHT/C++ compile succeeds.
2. all `TheUnit.MX50.*` tests execute and pass.
3. PR #19-#23 regression tests remain green.
4. a `UTUMissionPackageData` asset can be created in the editor.
5. a Briefing station can reference that asset and apply it in PIE.
6. marker positions appear in the expected grid cells.
7. marker selection/detail presentation works with mouse input.
8. hidden/removed marker selection recovers correctly.
9. planned-route data survives stow/re-raise.
10. available/offline feed states display correctly.
11. feed failover works in PIE.
12. no fake video frame is shown when no render target exists.
13. later Kill House floor-plan art can replace the prototype grid without changing marker data.
14. FPV integration consumes the video registry rather than coupling tablet code to flight physics.

No Unreal compile, automation execution, PIE validation, final tactical-map art, live route drawing or live FPV render target is claimed yet.
