# Third-Party Asset Registry

Record every externally sourced asset before it enters The Unit's content pipeline. A free download is not automatically redistributable source content.

## Rules

- Verify the license on the individual asset page, not only the marketplace/category page.
- Record author, source URL, license, download date, and attribution requirements.
- Keep original archives/source files in local `ExternalAssets/`, which is Git-ignored.
- Do not commit raw third-party models, textures, audio, or imported Unreal assets to this public repository unless the specific license permits source/repository redistribution.
- Commercial/game-use permission and raw-file redistribution permission are separate questions.
- Preserve license/readme files locally with the source asset.
- Prefer original or broadly licensed assets; do not use ripped, trademark-confusing, or unclear-provenance content.
- Before release, recheck every entry marked `Needs Review`.

## Registry

| Asset | Author | Source | License | Downloaded | Attribution | Intended Use | UE Destination | Source Redistribution Allowed? | Status |
|---|---|---|---|---|---|---|---|---|---|
| SWAT Operator (#4153582) | jeandiz; component creators below | [Listing](https://www.cgtrader.com/free-3d-models/character/man/swat-operator) | Listing: Custom License (no AI); author describes source components as CC-BY, version unspecified | User-supplied; inspected 2026-09-07 | Required; see credits below | Conversion candidate; character gear is joined | Not imported | Unknown pending exact terms | Needs Review |
| Bren 2 assault rifle (#3485426) | Studio-Svoboda | [Listing](https://www.cgtrader.com/free-3d-models/military/gun/bren-2) | Royalty Free License (no AI) as displayed 2026-09-07 | User-supplied; exact date unverified | Creator credit retained below | Candidate modular weapon art | Not imported | Not approved for public source redistribution | Needs Review |
| Private Military Contractor | jeandiz | [Matching listing](https://www.cgtrader.com/free-3d-models/military/military-character/private-military-contractor-8cb4b6a2-1d89-45be-b328-7f29ad58f653) | Editorial License (no AI) as displayed 2026-09-07 | User-supplied; exact date unverified | jeandiz | Assessment only; separate permission needed for commercial-game use | Not imported | Not approved | Needs Review |

## Attribution record: SWAT Operator

Source: [SWAT Operator](https://www.cgtrader.com/free-3d-models/character/man/swat-operator), assembled, edited, optimized, and retextured by [jeandiz](https://www.cgtrader.com/designers/jeandiz). Component credits transcribed from the author's listing and supplied by the user:

| Component | Original creator |
|---|---|
| Gloves | bobeer |
| NVGs | VassKacsoHunor |
| Helmet | Simon Coenen |
| Soldier Uniform | Bzovius |
| Boots | Albin |
| Balaclava | Shedmon |
| KRSV VECTOR | h1ggs |

The author identifies the component sources as Sketchfab and Blend Swap and describes them as CC-BY. Exact source URLs and license versions have not been supplied or verified. Do not substitute an assumed CC-BY version or treat this record as completed release clearance. Retain these credits, add the original asset/license links and required notices once verified, and include the resulting attribution in shipped credits when the assets are actually used.

The Unit modifications: none so far; inspection only. Record later mesh separation, rigging, material, or texture changes here before release. Attribution does not resolve the assembled listing's Custom License terms automatically.

## Other creator credits

- Bren 2 assault rifle: Studio-Svoboda, linked above. The Unit modifications: none; inspection only.
- Private Military Contractor: jeandiz, linked above. The Unit modifications: none; inspection only. Keep excluded from shipping until permission is resolved.

## Character customization acceptance gate

All character gear must be independently removable and replaceable. Verify separate equipment meshes, compatible skeleton/weights or attachment points, complete underlying body/clothing for supported unequipped states, and fit/clipping across supported combinations. A rigged assembled character or a source-credit list alone does not establish modular readiness. No asset has been approved or imported by this registry update.

## SWAT file inspection — 2026-09-07

User-supplied originals inspected read-only: `SWAT+Operator.fbx` (3,700,748 bytes), `SWAT+Operator+CGTrader.blend` (16,105,872 bytes), and `Textures.zip` (216,936,820 bytes). FBX structure, Blender datablocks, and archive filenames were parsed; no embedded scripts were executed and no files were imported into Unreal.

- FBX: 32 mesh objects. The character uses the single `sol_8_low` mesh; the other 31 named mesh objects are weapon parts. The skeleton uses `mixamorig:` bone names.
- Blender 3.3 file: the same 32 mesh objects plus the `SWAT` armature. It does not preserve separate character equipment objects. A combined mesh may still contain separable geometry islands, but these have not been classified into gear slots.
- Textures: black/camo character variants and beige/black Vector variants. No license/readme files were found in the archive; no Blender text datablocks were found. Existing attribution and exact-license review remain applicable.
- Verdict: not ready for fully interchangeable character gear as supplied. Retain as a conversion candidate, not an approved modular base.
- Required next asset work: visually identify and separate equipment while preserving skin weights and UVs; verify underlying body/clothing coverage; repair or replace missing underlying surfaces if found; establish attachment/rig compatibility and test gear swaps for clipping. Body completeness and deformation quality are not established by this structural inspection.

The source assets remain unmodified and outside Git. This inspection adds no character, weapon, or gameplay implementation.

## Status values

- `Approved` — license checked and use/handling requirements understood.
- `Needs Review` — candidate only; do not ship or publish.
- `Rejected` — license, provenance, quality, or compatibility issue.
