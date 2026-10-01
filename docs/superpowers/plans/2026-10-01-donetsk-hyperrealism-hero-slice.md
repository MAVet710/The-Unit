# Donetsk Hyperrealism Hero Slice Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the visible Donetsk prototype layer with a production-quality hero slice that meets the approved Bodycam visual target, preserves OPERATOR-style first-person coherence, and does not regress Tarkov-style raid systems.

**Architecture:** Keep the deterministic procedural generator as hidden collision and gameplay structure. Layer validated high-fidelity meshes, physically plausible materials, decals, environment dressing, lighting, and repeatable visual evidence on top.

**Tech Stack:** Unreal Engine 5.7.4, C++, Unreal Python, DX12, SM6, Nanite, Lumen, Virtual Shadow Maps, TSR, PowerShell, Meshy/FBX.

**Spec:** `docs/superpowers/specs/2026-10-01-donetsk-hyperrealism-design.md`

## Global Constraints

- Preserve gameplay layout, collision, network determinism, extraction geometry, navigation, and raid authority.
- Bodycam is the environment-fidelity reference, OPERATOR the embodiment reference, and Tarkov the raid-system reference.
- Do not copy proprietary assets, maps, textures, audio, or code from reference games.
- Hide visible primitive geometry whenever a validated production replacement exists.
- Ultra validation is 2560x1440 on the RTX 4070 Ti SUPER machine; normal beta hardware retains a scalable High path.
- A street-level frame that still reads as a graybox fails.

## Meshy Acquisition Rule

Visible environment assets use this order:

1. Search Meshy Community first for an existing realistic asset with appropriate commercial-use licensing.
2. Record the candidate source URL, license, creator/source metadata, intended role, and local file name before import.
3. Reject candidates that fail first-person silhouette, proportion, texture, or realism review even if the license is acceptable.
4. When Community does not provide a suitable asset, use Meshy AI generation from a focused text prompt or available reference images.
5. Meshy AI generation workspace: `ExternalAssets/DonetskHighFidelity`; CLI bookkeeping lives in `ExternalAssets/DonetskHighFidelity/meshy_output`.
6. For a new text-generated asset, run `meshy make "<description>" --dry-run` first to inspect the generation chain/credit estimate, then submit once. Use 4K PBR refinement for hero/near-field assets.
7. Reuse an existing Meshy task for retexture, remesh, resize, or format conversion instead of paying for a fresh generation when the geometry is already acceptable.
8. Download GLB for inspection and FBX when needed by the Unreal import pipeline. Normalize scale in Unreal after import and preserve the source/task receipt.
9. Community and generated assets pass the same Task 1/Task 2 gates before they may replace a visible prototype.

Current Community search already confirms useful Soviet/Post-Soviet and Ukrainian building categories plus CC0 examples; Community remains the first source rather than generating duplicates.

## Review Focus

1. Reimport/scale drift: re-running import or normalization must not move approved assets outside the 3.5 percent scale tolerance. Task 1 adds idempotence checks.
2. Visible art changing gameplay collision: production visuals stay non-colliding and deterministic collision signatures stay stable. Task 5 tests this.
3. Broken PBR wiring: normal, roughness, metallic, opacity, and sRGB settings must be correct and no asset may fall back to WorldGridMaterial. Task 3 validates this.
4. Repeated or nondeterministic dressing: variation must look organic but produce the same visual signature on peers. Task 6 uses fixed-seed composition and tests it.
5. Ultra settings leaking into normal beta defaults: hardware Lumen and Epic-class overrides exist only in the validation launch path. Task 7 tests normal fallback.

## File Structure

- Modify `Tools/AssetPrep/donetsk_high_fidelity_assets.json`.
- Modify `Tools/Unreal/normalize_donetsk_high_fidelity_scale_editor.py`.
- Create `Tools/Tests/test_donetsk_asset_receipts.py`.
- Create `Tools/Unreal/validate_donetsk_visual_assets_editor.py`.
- Create `Tools/Unreal/build_donetsk_hyperreal_materials_editor.py` and `validate_donetsk_materials_editor.py`.
- Create `Tools/Reference/donetsk_surface_source_ledger.json`.
- Create `Tools/Unreal/import_donetsk_decals_editor.py` and `validate_donetsk_decals_editor.py`.
- Modify `TU_DonetskDistrictGenerator` header/source and Donetsk tests.
- Modify `TU_WorldLighting` only where required for configurable neutral daylight.
- Create `Scripts/RunDonetskUltraValidation.ps1`.
- Create `TUDonetskVisualProbe` header/source/tests and validation-view JSON.
### Task 1: Repair and harden the high-fidelity scale gate

**Files:**
- Modify: `Tools/Unreal/normalize_donetsk_high_fidelity_scale_editor.py`
- Modify: `Tools/AssetPrep/donetsk_high_fidelity_assets.json`
- Create: `Tools/Tests/test_donetsk_asset_receipts.py`
- Evidence: `ExternalAssets/DonetskHighFidelity/scale_receipt.json`

**Interfaces:** Consume manifest `target_name`, `destination`, `scale_mode`, `target_cm`, `source_type`, `source_url`, `license`, and optional `meshy_task_id`; produce `the-unit/donetsk-high-fidelity-scale/v2` with all required assets passing, dimensions, build-scale state, iteration count, and idempotence factor.

- [ ] **Step 1: Write failing receipt tests.** Assert manifest version 3, 10 required assets, success status, every error <= 0.035, second-pass factor within 0.005 of 1.0, and `SM_Donetsk_Khrush_5F_12` between 1447.5 and 1552.5 cm.
- [ ] **Step 2: Run `python -m unittest Tools.Tests.test_donetsk_asset_receipts -v`.** Expected: FAIL on the current failed v1 receipt.
- [ ] **Step 3: Make scale normalization converge.** Rebuild/reload before post-scale measurement, permit at most three bounded correction passes, record pre/post build scale and idempotence, and fail hard after the third miss. Do not special-case an asset by name.
- [ ] **Step 4: Run the UE 5.7 normalization editor script twice.** Expected after run two: 10/10 pass and all idempotence factors approximately 1.0.
- [ ] **Step 5: Run the offline test plus `Scripts/ValidateTheUnit.ps1 -TestFilter "TheUnit.Maps.Donetsk."`.** Expected: PASS.
- [ ] **Step 6: Commit:** `fix: harden Donetsk asset scale gate`.
### Task 2: Add an in-engine production-asset quality receipt

**Files:**
- Create: `Tools/Unreal/validate_donetsk_visual_assets_editor.py`
- Modify: `Tools/AssetPrep/donetsk_high_fidelity_assets.json`
- Modify: `Tools/Tests/test_donetsk_asset_receipts.py`
- Evidence: `ExternalAssets/DonetskHighFidelity/visual_quality_receipt.json`

**Interfaces:** Produce `the-unit/donetsk-visual-quality/v1` with bounds, material-slot count, Nanite state, default-material detection, source hash, and manual-review state for every imported production asset.

- [ ] **Step 1: Add failing tests** for exactly 10 target names, at least one material slot, no `WorldGridMaterial`, positive bounds, scale still in tolerance, and Nanite enabled for opaque building/rubble/prop roles.
- [ ] **Step 2: Run the receipt tests.** Expected: FAIL because the quality receipt does not exist.
- [ ] **Step 3: Implement the editor validator.** It must load every manifest asset, emit one result per target, and mark the whole receipt failed if an asset is missing or invalid. Any missing or rejected visible role is added to a sourcing queue that records `community_search`, `community_selected`, `generate_required`, or `approved` rather than silently falling back to prototype art.
- [ ] **Step 4: Generate the receipt and rerun tests.** Mechanical checks must pass; `manual_review` remains `pending` until Task 8.
- [ ] **Step 5: Commit:** `test: gate Donetsk production visual assets`.
### Task 3: Build the physically based Donetsk surface system

**Files:**
- Create: `Tools/Reference/donetsk_surface_source_ledger.json`
- Create: `Tools/Unreal/build_donetsk_hyperreal_materials_editor.py`
- Create: `Tools/Unreal/validate_donetsk_materials_editor.py`
- Generate: `Content/TheUnit/Donetsk/Materials/`
- Evidence: `Saved/Automation/DonetskVisual/material_receipt.json`

**Interfaces:** Produce `M_Donetsk_Surface_Master`, `M_Donetsk_Glass_Master`, and `M_Donetsk_Foliage_Master`, plus instances for asphalt, paving, aged concrete, curb concrete, plaster, panel warm/cool, brick, galvanized metal, rusted steel, wood, soil, and wet ground.

- [ ] **Step 1: Run a validator that expects the new masters and instances.** Expected: FAIL before they exist.
- [ ] **Step 2: Create the source ledger.** Every surface must point to an owned, CC0, or original generated source with local files, source URL when applicable, license, physical surface, resolution, and approval state. No reference-game textures may be used.
- [ ] **Step 3: Build reusable masters.** Surface parameters: base color, normal, roughness, metallic, macro tint, macro roughness, detail normal strength, wetness, UV scale, per-instance variation, world-aligned fallback, and DBuffer compatibility.
- [ ] **Step 4: Validate texture settings.** Normals must use normal-map compression and non-sRGB; roughness/metallic non-sRGB; non-metals metallic near zero; no WorldGridMaterial; glass and foliage use dedicated masters.
- [ ] **Step 5: Apply approved instances to visible Donetsk assets, then rerun Task 1 and 2 receipts.** Expected: scale and asset gates still pass.
- [ ] **Step 6: Commit:** `feat: add Donetsk hyperreal PBR materials`.
### Task 4: Build the DBuffer decal library

**Files:**
- Create: `ExternalAssets/DonetskDecals/`
- Create: `Tools/Reference/donetsk_decal_source_ledger.json`
- Create: `Tools/Unreal/import_donetsk_decals_editor.py`
- Create: `Tools/Unreal/validate_donetsk_decals_editor.py`
- Generate: `Content/TheUnit/Donetsk/Decals/`

**Interfaces:** Produce `M_Donetsk_Decal_Master` and at least 12 validated decal instances across facade damage, water/grime, rust, road wear, signage remnants, and fictional mission-impact categories.

- [ ] **Step 1: Write/run the decal validator before assets exist.** Require DBuffer-compatible material domain/blend, non-empty texture inputs, opacity where needed, source-ledger coverage, and authored physical-size limits. Expected: FAIL.
- [ ] **Step 2: Author or source permissively licensed decal maps** for cracked plaster, chipped paint, exposed substrate, soot, rain streaks, rust drips, patched asphalt, asphalt cracking, tire wear, curb grime, utility marking, faded signage, and water staining.
- [ ] **Step 3: Import master and instances, then validate.** Expected: PASS with source-ledger coverage for every decal.
- [ ] **Step 4: Commit:** `feat: add Donetsk surface decal library`.
### Task 5: Enforce hidden collision versus visible production art

**Files:**
- Modify: `Source/TheUnit/Public/TU_DonetskDistrictGenerator.h`
- Modify: `Source/TheUnit/Private/TU_DonetskDistrictGenerator.cpp`
- Modify: `Source/TheUnit/Private/Tests/TUDonetskReferenceTests.cpp`
- Modify: `Source/TheUnit/Private/Tests/TUGeneratedGeometryTests.cpp`

**Interfaces:** Add `GetProductionVisualComponentCount()`, `GetVisiblePrimitiveFallbackCount()`, and `GetGeneratedVisualSignature()`. Production visuals remain `ECollisionEnabled::NoCollision`.

- [ ] **Step 1: Write failing separation tests.** Assert collision count stays above 100, production visual count is positive, every production visual has collision disabled, two generators have identical visual signatures, visible primitive fallback count is at most 12, and collision signatures remain identical.
- [ ] **Step 2: Load validated production meshes** for imported building families, bus shelter, sedan, rubble, trees, and existing civic/station assets.
- [ ] **Step 3: Hide prototype rendering whenever production art exists.** Architectural masses, opening plates, cube props, cube rubble, and placeholder furniture must not remain visible behind replacements. Road/ground carriers may stay visible only when intentionally final.
- [ ] **Step 4: Implement the visual-signature methods** from mesh asset path plus relative transform, independent of memory address and registration order.
- [ ] **Step 5: Run `ValidateTheUnit.ps1` with Donetsk and `GeneratedGeometryIdentity` filters.** Expected: PASS.
- [ ] **Step 6: Commit:** `feat: separate Donetsk collision from production art`.
### Task 6: Assemble deterministic hero-slice dressing

**Files:**
- Modify: `Source/TheUnit/Public/TU_DonetskDistrictGenerator.h`
- Modify: `Source/TheUnit/Private/TU_DonetskDistrictGenerator.cpp`
- Modify: `Source/TheUnit/Private/Tests/TUDonetskReferenceTests.cpp`
- Add/import validated props under: `Content/TheUnit/Donetsk/Environment/`

**Interfaces:** Add `BuildHeroSliceDressing()` and use fixed `FRandomStream` seed `710` for bounded visual variation only. Never use nondeterministic random calls for replicated layout.

- [ ] **Step 1: Write failing composition tests.** Require deterministic visual signatures, production bus shelter/sedan/rubble/drain or manhole/wet element presence, non-identical tree scale/yaw, unchanged collision signature, and no blocking production visual.
- [ ] **Step 2: Build the approved benchmark area** with one boulevard segment, apartment frontage, civic edge, transit stop, rubble lane, vegetation, civilian vehicle, ground transitions, curb/drain detail, and utility detail.
- [ ] **Step 3: Photo-match named hero architecture.** Cross-check Artema 60, the selected residential frontage, and the civic edge against `docs/DONETSK_REFERENCE_MAP.md` and its cited reference set. Run `TheUnit.Maps.Donetsk.` plus `TUDonetskArtema60Tests`; generic Soviet assets may remain only as secondary filler.
- [ ] **Step 4: Replace every weak visible box/cylinder prop in the hero slice.** If a Meshy asset looks melted, toy-like, or low-detail at first-person distance, regenerate or replace it before proceeding.
- [ ] **Step 5: Apply localized decals** according to plausible material history rather than uniform scatter.
- [ ] **Step 6: Run Donetsk and generated-geometry automation.** Expected: PASS.
- [ ] **Step 7: Commit:** `feat: build Donetsk hero realism slice`.
### Task 7: Add opt-in Ultra validation lighting and rendering

**Files:**
- Modify: `Source/TheUnit/Public/TU_WorldLighting.h`
- Modify: `Source/TheUnit/Private/TU_WorldLighting.cpp`
- Modify: `Config/DefaultEngine.ini` only for project support required by optional hardware Lumen
- Create: `Scripts/RunDonetskUltraValidation.ps1`
- Create: `Tools/Tests/test_donetsk_ultra_profile.py`

**Interfaces:** Normal game startup retains scalable user quality. The Ultra launcher requests 2560x1440, Epic-class rendering, TSR, and hardware Lumen only where supported.

- [ ] **Step 1: Write a failing configuration test.** Assert normal `DefaultGameUserSettings.ini` does not force Epic/Cinematic, Ultra launcher contains 2560x1440 and explicit Epic-class GI/reflection/shadow/texture/foliage/post-process/view-distance overrides, and hardware Lumen appears only in the validation path.
- [ ] **Step 2: Expose only needed neutral-daylight parameters** in `TU_WorldLighting`: sun intensity/temperature, skylight intensity, fog density/falloff, exposure bias. No cinematic grade or permanent fisheye.
- [ ] **Step 3: Enable only project-level renderer support needed for optional hardware ray tracing.** Normal play must remain valid with software Lumen.
- [ ] **Step 4: Run the Ultra launcher and verify DX12/SM6, Nanite, Lumen, VSM, 2560x1440, and no fatal RT fallback errors. Then run normal High without Ultra overrides.
- [ ] **Step 5: Run config tests plus `TheUnit.Presentation.` and `TheUnit.Maps.Donetsk.` automation.** Expected: PASS.
- [ ] **Step 6: Commit:** `feat: add Donetsk ultra visual validation profile`.
### Task 8: Add repeatable visual evidence and screenshot gates

**Files:**
- Create: `Source/TheUnit/Public/TUDonetskVisualProbe.h`
- Create: `Source/TheUnit/Private/TUDonetskVisualProbe.cpp`
- Create: `Source/TheUnit/Private/Tests/TUDonetskVisualProbeTests.cpp`
- Create: `Tools/Reference/donetsk_visual_validation_views.json`
- Evidence: `Saved/Automation/DonetskVisual/<capture-id>/`

**Interfaces:** Opt-in command line `-TUDonetskVisual=<safe-id>`. Evidence is 10 PNG screenshots plus `result.json`. Normal launches never activate the probe.

- [ ] **Step 1: Write failing safety/viewpoint tests.** Accept safe IDs; reject traversal, absolute paths, quotes, and empty IDs; require exactly 10 approved viewpoint names; verify the probe is inactive without its command-line flag.
- [ ] **Step 2: Implement the development-only probe** by reusing the safe capture pattern from `UTUBetaEntryProbe` and `ATUHandlingCapture`.
- [ ] **Step 3: Record evidence metadata:** viewport resolution, scalability values, Nanite/Lumen/VSM flags, map name, production visual count, visible primitive fallback count, screenshot paths, and pass/fail reason.
- [ ] **Step 4: Run `ValidateTheUnit.ps1 -TestFilter "TheUnit.Maps.Donetsk.Visual"`.** Expected: PASS.
- [ ] **Step 5: Capture all 10 views with the Ultra launcher.** Require non-empty PNGs and 2560x1440 evidence.
- [ ] **Step 6: Human visual review at full size.** Reject graybox reads, cube architecture, melted AI meshes, flat/noisy placeholder materials, obvious tiling, floating props, implausible scale, repeated patterns, or broken exposure/shadows.
- [ ] **Step 7: Record manual approval.** Set the visual-quality receipt `manual_review` to `approved` with capture ID and commit hash.
- [ ] **Step 8: Commit:** `test: add Donetsk visual acceptance evidence`.
### Task 9: Verify first-person presentation inside the hero slice

**Files:**
- Modify only if evidence proves a defect: `Source/TheUnit/Private/Tests/TUPresentationTests.cpp`
- Modify only if evidence proves a defect: `Source/TheUnit/Private/Tests/TURealismContactTests.cpp`
- Modify only if evidence proves a defect: `Source/TheUnit/Private/Tests/TUHandlingAnimationTests.cpp`
- Production handling fixes go to the smallest owning component, never the map generator.

**Interfaces:** Existing weapon/body authority remains unchanged. Environment art must not drive ammunition, weapon action state, or authoritative shot direction.

- [ ] **Step 1: Run existing handling/presentation tests before changing code.** Run `ValidateTheUnit.ps1 -TestFilter "TheUnit.Handling.;TheUnit.Presentation.TemplatePOVClearance"`. Expected: PASS.
- [ ] **Step 2: Capture the weapon-in-world validation view.** Inspect shoulder relationship, optic alignment, support hand, muzzle clearance near cover, weapon lighting/contact shadows, visible body, and interior/exterior exposure.
- [ ] **Step 3: If evidence proves a defect, write a failing test in the owning test file first.** Never use a camera offset to conceal an authoritative weapon-placement defect.
- [ ] **Step 4: Make the smallest handling fix, then rerun handling, presentation, and Donetsk visual tests.** Expected: PASS.
- [ ] **Step 5: Commit only if code changed:** `fix: align first-person presentation with hero slice`.
### Task 10: Full regression, performance evidence, and packaged beta

**Files:**
- Reuse existing package scripts under `E:\TheUnitWork\beta-entry-20260927\`
- Evidence: `E:\TheUnitWork\donetsk-hyperrealism-evidence\<commit>\`
- Output: a new versioned Windows build directory; never overwrite the last known-good package.

**Interfaces:** Package identity includes the final git commit. Evidence includes test logs, scale/quality/material/decal receipts, Meshy Community source records and AI task IDs for newly generated assets, visual captures, runtime log, and performance summary.

- [ ] **Step 1: Run the full suite:** `powershell -ExecutionPolicy Bypass -File Scripts/ValidateTheUnit.ps1`. Expected: editor build and all `TheUnit.*` tests PASS.
- [ ] **Step 2: Run Ultra hero-slice validation.** Record average frame time, 1 percent low if available, CPU/GPU bottleneck indication, active resolution/features, and capture ID. Do not lower visual quality to hide an art defect.
- [ ] **Step 3: Run a normal High-profile launch** and verify the map remains playable without Ultra overrides or a hardware-RT requirement.
- [ ] **Step 4: Package a new Windows beta** using the existing pipeline. Do not overwrite `main-release-donetsk-production-v3-20260929`.
- [ ] **Step 5: Run packaged game smoke and two-player/network smoke.** Extraction, persistence, mission flow, collision, and handling must not regress.
- [ ] **Step 6: Final evidence review.** Accept only when all automated gates pass, all 10 baseline scale assets and every new replacement pass, visual/material/decal receipts pass, Meshy Community/generated source records are complete, manual screenshot review is approved, package smoke/network gates pass, and the hero slice no longer reads as a prototype.

## Execution Order

Tasks 1 and 2 are the asset gate. Task 3 and 4 establish surface fidelity. Tasks 5 and 6 convert the map from prototype rendering to production art. Task 7 tunes the high-end renderer without changing normal requirements. Task 8 is the visual acceptance gate. Task 9 protects OPERATOR-style first-person coherence. Task 10 produces the playable Windows beta and evidence bundle.

Do not propagate the hero-slice art language to the rest of Donetsk until Task 8 has been manually approved.
