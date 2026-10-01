# Donetsk Hyperrealism Design

## Purpose

Bring The Unit's Donetsk environment to a hyperrealistic visual standard while preserving the existing gameplay layout, collision, network determinism, and extraction-shooter systems.

The visual north star is:
- Bodycam for photorealistic environment rendering and optical response.
- OPERATOR for first-person body presence, weapon embodiment, stance, and physical interaction.
- Escape from Tarkov for raid structure, inventory risk, ballistics depth, equipment state, missions, and extraction flow.

These games are design references only. No proprietary assets, maps, textures, audio, code, or other protected content will be copied.

## Success Criteria

A street-level screenshot must not read as a graybox or procedural prototype.
Visible architecture must no longer be dominated by primitive cube silhouettes.
Surfaces must exhibit physically plausible material response and non-repeating wear.
The first-person weapon must feel attached to a human body instead of floating in camera space.
Gameplay collision and authoritative raid behavior must remain stable while visible art is replaced.

## Current-State Findings

The rendering foundation is suitable for a high-end target: DirectX 12, Shader Model 6, Nanite, Lumen GI/reflections, Virtual Shadow Maps, mesh distance fields, TSR, and volumetric atmosphere/fog.

The primary visual bottleneck is not missing renderer features. It is the visible art layer.\n\nThe current Donetsk generator still relies heavily on procedural AddBox(...) geometry for roads, civic details, collision proxies, mission damage, street furniture, and architectural rhythm.
The locally generated production environment props are also largely built from boxes and cylinders.
The generated material textures use procedural noise and do not yet encode enough real-world surface structure, history, or localized wear to meet the target.

A second, higher-fidelity asset tier already exists under ExternalAssets/DonetskHighFidelity.
Ten imported Meshy/FBX assets have import receipts, including Soviet residential buildings, street trees, a bus shelter, civilian sedan, and rubble source.
Nine of ten currently pass the recorded scale gate.
SM_Donetsk_Khrush_5F_12 is approximately 6.7 percent under its 1500 cm target and must be corrected before approval.

The default user settings also target 1280x720 and scalability level 2. That is acceptable for fallback testing, not for the visual acceptance build on the RTX 4070 Ti SUPER development machine.

## Core Architecture

### 1. Separate gameplay skeleton from visible world

The procedural Donetsk system remains authoritative for collision, traversal bounds, extraction geometry, navigation, replicated layout, deterministic cover footprints, and mission placement anchors.
Visible primitive geometry must be hidden whenever a production visual replacement exists.
The visible layer is built from authored or generated production assets with validated scale, materials, UVs, normals, silhouettes, and collision separation.

This prevents visual fidelity work from destabilizing gameplay or networking.

### 2. Hero-slice-first production

Do not attempt to polish the entire district at once.\n\nCreate one hero-quality slice containing one boulevard segment, one apartment frontage, one civic edge, one transit stop, one damaged/rubble lane, representative vegetation, a representative civilian vehicle, and representative ground-material transitions.

This slice becomes the benchmark scene. No district-wide propagation happens until the hero slice passes visual review.

### 3. Asset quality gate

Every visible production asset must pass:
- real-world scale check
- silhouette check at 5 m, 25 m, and 100 m
- first-person texture density check
- UV/stretching check
- material-slot sanity check
- normals/tangent check
- shadow quality check
- Nanite suitability check
- repetition check when instanced
- gameplay-space fit check
- license/source receipt check

Assets that look synthetic, melted, toy-like, low-detail, or visibly AI-generated in first person are rejected and regenerated, replaced, or hand-corrected.

## Material System

Build shared Unreal master materials rather than one-off flat materials.

Required surface families: aged concrete, painted plaster/stucco, Soviet panel facade, brick, asphalt, paving, curb concrete, galvanized and painted metal, rusted steel, dirty architectural glass, wood, vegetation, soil, and wet surface/puddle response.

Each relevant master material should support base color, normal, roughness, metallic where physically appropriate, macro color variation, macro roughness variation, dirt accumulation, leak/streak masks, edge wear where appropriate, wetness control, detail normal, world-aligned or tri-planar fallback where UVs are weak, vertex/per-instance variation, and decal compatibility.

Material values must remain physically plausible. Realism comes from structured variation, not exaggerated contrast.\n\n## Decal System

Use DBuffer-compatible decals for localized history and repetition breakup.

Required decal families include cracked plaster, chipped facade paint, exposed substrate, soot, rain streaks, rust drips, patched asphalt, road cracking, tire wear, curb grime, utility markings, old signage remnants, water staining, and mission-fiction impact/spall marks.

Decals should tell a plausible material story. Random noise without spatial logic is not acceptable.

## Environment Dressing

Replace uniform placement with believable clustered composition.

The hero slice should include weeds in curb/pavement seams, drains and manholes, overhead utility/transit wires, utility cabinets, fencing, lamp posts, curb damage, trash and small debris, pallets and maintenance clutter, parked or abandoned civilian vehicles, bus-stop furniture, puddles in believable low areas, exposed soil/root zones, varied tree scale/rotation/spacing/health, dark interior silhouettes/window variation, rooftop mechanical detail, and mission-specific rubble authored separately from clean civilian architecture.

Clutter density must support navigation and combat readability. Visual realism must not destroy tactical legibility.

## Architecture

Hero buildings must be photo-matched where references are strong enough.

Priority families:
1. Artema Street 60
2. central civic frontage
3. Khrushchev-era residential blocks
4. Brezhnev-era residential blocks
5. Stalin-era frontage
6. railway-station reference volumes

The existing Donetsk architecture bible remains the source of truth for reference provenance and dimensional confidence.
Generic Soviet-looking assets are acceptable only as secondary filler, not as named hero landmarks.\n\n## Lighting and Atmosphere

Preserve the current Lumen/Nanite/VSM foundation.
Target naturalistic urban daylight rather than stylized cinematic grading.

Desired characteristics include believable sky luminance, coherent exterior/interior exposure transitions, cool concrete and neutral daylight, soft but readable shadow penumbra, restrained bloom, subtle atmospheric haze, believable wet-surface reflections, strong contact shadows, usable dark interiors without crushed-black gameplay, and no permanent bodycam fisheye distortion.

Bodycam informs the realism target, not a mandatory camera gimmick.

## High-End Development Profile

Create a development validation profile intended for the RTX 4070 Ti SUPER machine.

Target 2560x1440 validation resolution with Epic-class textures, shadows, global illumination, reflections, foliage, and post processing, plus TSR quality appropriate for a clean 1440p presentation.

Evaluate Lumen hardware ray tracing where it materially improves interior lighting, reflections, and close-range fidelity without breaking the performance target.

A separate scalable High profile remains required for beta hardware. The Ultra development target must not become the minimum hardware requirement.

## First-Person Presentation

OPERATOR is the primary reference philosophy for first-person embodiment.

Required behavior includes visible body coherence, stock-to-shoulder relationship, believable cheek weld, correct optic alignment, correct support-hand placement, physically plausible reload hand paths, magazine/chamber state reflected in animation, high-ready and low-ready states, stance-linked weapon presentation, procedural lean linked to body position, muzzle obstruction/wall compression, weapon reaction to door frames and tight spaces, and grounded sway rather than camera-only oscillation.

The weapon must behave as an object carried by a body, not as a HUD element.\n\n## Tarkov-Inspired Gameplay Boundary

The visual rebuild must preserve and support the existing extraction-shooter direction: persistent stash, raid loadout risk, modular weapons, magazine state, chamber state, ammunition packing, ammunition-specific ballistic behavior, armor/plate interaction, equipment weight, medical consequences, mission items, loot value, extraction conditions, raid-result persistence, and solo/two-player co-op.

Visual work must not simplify these systems.

## Validation

### Screenshot gate

Capture and review:
1. close facade at first-person distance
2. sidewalk and curb detail
3. 50-100 m streetscape
4. long boulevard sightline
5. civic-space view
6. damage/rubble composition
7. vegetation silhouette
8. wet/roughness response
9. interior-to-exterior exposure transition
10. weapon-in-world lighting and body relationship

Any frame that still reads as a graybox fails.

### Play gate

Verify walking, sprinting, leaning, stance transitions, cover readability, collision alignment with visible art, no floating props, no miniature/oversized assets, no obvious texture tiling, no repeated identical dressing patterns, no severe Lumen/Nanite/VSM artifacts, stable navigation, stable extraction paths, and acceptable performance on the development profile.\n\n## Production Sequence

1. Repair the existing imported high-fidelity scale failure.
2. Audit all ten imported high-fidelity assets in-engine.
3. Build the shared material system.
4. Build the decal library and placement workflow.
5. Assemble the hero slice using production visuals over hidden collision.
6. Replace weak props with higher-quality Meshy/generated/authored assets.
7. Add physically motivated environment dressing.
8. Tune lighting, exposure, atmosphere, and high-end scalability.
9. Validate first-person weapon/body presentation in the hero slice.
10. Run screenshot and play gates.
11. Propagate the approved quality language across the remaining playable district.
12. Package and validate a new Windows beta build.

## Non-Goals

This pass does not reproduce a real military deployment or live tactical map, copy proprietary content from reference games, require every building to become enterable immediately, require full cinematic ray-traced rendering on all beta hardware, sacrifice gameplay readability for clutter, or replace the deterministic collision/layout system unless a gameplay defect requires it.

## Definition of Done

Donetsk is visually acceptable for this phase only when the hero slice no longer resembles a prototype from normal first-person play, the visible art is production-quality, the environment supports the Bodycam/OPERATOR visual target, and Tarkov-style gameplay systems remain intact and testable.
