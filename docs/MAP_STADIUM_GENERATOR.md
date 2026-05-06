# MAP — Stadium Generator (Phase 1D.2)

## Purpose

`ATU_StadiumGenerator` is a drag-and-drop procedural graybox actor for Unreal Engine 5.7 that builds an asymmetrical, old-park-inspired tactical training map in-editor.

The generator is intentionally **inspired by public geometry only** and avoids licensed branding, logos, sponsor references, and trademarked visual identity.

## How to Use

1. Open any map in Unreal Editor.
2. Drag `TU_StadiumGenerator` into the scene.
3. In Details, tune generation toggles and field dimensions.
4. The actor regenerates automatically in `OnConstruction()`.

Key controls:
- `FieldScale` scales 1 ft -> 30.48 UU conversion.
- `bGenerateField`, `bGenerateInfield`, `bGenerateOutfieldWalls`, `bGenerateMonsterWall`
- `bGenerateDugouts`, `bGenerateLowerBowl`, `bGenerateUpperDeck`, `bGenerateExteriorShell`
- `bGenerateGameplayMarkers`, `bGenerateDebugLabels`

## Dimensions Used (Public Reference Values)

- Left field line: 310 ft
- Left-center: 379 ft
- Center field: 390 ft
- Deep center/triangle: 420 ft
- Deep right: 380 ft
- Right field line: 302 ft
- Left wall (monster-style): 37 ft
- Center wall: 17 ft
- Right wall: 5 ft

All values are editable in Details and converted with:
- `UnrealUnits = Feet * 30.48 * FieldScale`

## Legal / IP Guardrails

- No MLB, team, or venue logos.
- No sponsor signage or direct brand marks.
- No trademarked paint/brand color matching requirements.
- No exact one-to-one replica intent.
- Geometry is tactical graybox abstraction suitable for training/simulation blocking.

## Known Limitations

- Uses only `/Engine/BasicShapes/Cube.Cube` and marker/debug components.
- Seating and shell are macro blockouts, not production art.
- No navmesh-specific carving, crowd simulation, or lighting setup.
- No runtime LOD/HISM optimization pass yet.

## Future Improvements

- Replace raw cubes with modular stadium kit pieces.
- Add optional concourse routing presets and collision channels.
- Add data asset profiles for multiple asymmetrical park variants.
- Add objective scripting hooks and mission seed randomization.
- Add per-section naming conventions for encounter tooling.
