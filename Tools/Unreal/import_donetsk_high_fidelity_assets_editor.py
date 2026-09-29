import unreal
import json
import hashlib
from pathlib import Path
from datetime import datetime, timezone

ROOT = Path(r"C:\Users\ndasi\OneDrive\Documents\GitHub\The-Unit")
SOURCE_ROOT = ROOT / "ExternalAssets" / "DonetskHighFidelity"
MANIFEST_PATH = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
RECEIPT_PATH = SOURCE_ROOT / "import_receipt.json"

manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

missing = []
imported = []
failed = []

for spec in manifest["assets"]:
    src = SOURCE_ROOT / spec["expected_source"]
    if not src.exists():
        missing.append(str(src))
        continue

    task = unreal.AssetImportTask()
    task.filename = str(src)
    task.destination_path = spec["destination"]
    task.destination_name = spec["target_name"]
    task.automated = True
    task.replace_existing = True
    task.save = True

    try:
        asset_tools.import_asset_tasks([task])
        found_mesh = False

        for object_path in task.imported_object_paths:
            asset = unreal.EditorAssetLibrary.load_asset(object_path)
            if isinstance(asset, unreal.StaticMesh):
                found_mesh = True
                try:
                    ns = asset.get_editor_property("nanite_settings")
                    ns.enabled = True
                    asset.set_editor_property("nanite_settings", ns)
                except Exception as exc:
                    print("NANITE_WARN", object_path, exc)

                try:
                    asset.set_editor_property("light_map_resolution", 256)
                except Exception:
                    pass

                # Gameplay collision is deliberately maintained by the deterministic
                # generated collision layer. High-fidelity art should not introduce
                # accidental blocking volumes that change traversal.
                try:
                    asset.set_editor_property(
                        "customized_collision",
                        False,
                    )
                except Exception:
                    pass

                unreal.EditorAssetLibrary.save_loaded_asset(asset)

        if not found_mesh:
            raise RuntimeError(f"No StaticMesh imported for {src}")

        digest = hashlib.sha256(src.read_bytes()).hexdigest()
        imported.append({
            "target_name": spec["target_name"],
            "destination": spec["destination"],
            "source_file": spec["expected_source"],
            "source_url": spec["source_url"],
            "license": spec["license"],
            "sha256": digest,
            "imported_object_paths": list(task.imported_object_paths),
        })
        print("HF_IMPORT_OK", spec["target_name"], task.imported_object_paths)

    except Exception as exc:
        failed.append({"target_name": spec["target_name"], "error": str(exc)})
        print("HF_IMPORT_FAIL", spec["target_name"], exc)

receipt = {
    "schema": "the-unit/donetsk-high-fidelity-import/v1",
    "generated_at_utc": datetime.now(timezone.utc).isoformat(),
    "manifest_version": manifest["version"],
    "required_count": len(manifest["assets"]),
    "imported_count": len(imported),
    "missing_count": len(missing),
    "failed_count": len(failed),
    "assets": imported,
    "missing": missing,
    "failed": failed,
}

SOURCE_ROOT.mkdir(parents=True, exist_ok=True)
RECEIPT_PATH.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")

print(
    f"DONETSK_HIGH_FIDELITY_IMPORT imported={len(imported)} "
    f"missing={len(missing)} failed={len(failed)} receipt={RECEIPT_PATH}"
)

if missing or failed:
    unreal.log_error("Donetsk high-fidelity import incomplete. Packaged release must not proceed.")
    unreal.SystemLibrary.quit_editor()
    raise RuntimeError("Donetsk high-fidelity import incomplete")

unreal.SystemLibrary.quit_editor()
