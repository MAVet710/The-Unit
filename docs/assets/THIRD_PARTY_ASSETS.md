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
| _Example_ | _Creator_ | _URL_ | _License name_ | YYYY-MM-DD | None/Required | Kill House prop | `/Game/TheUnit/...` | Yes/No/Unknown | Needs Review |

## Status values

- `Approved` — license checked and use/handling requirements understood.
- `Needs Review` — candidate only; do not ship or publish.
- `Rejected` — license, provenance, quality, or compatibility issue.
