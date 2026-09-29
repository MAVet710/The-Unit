import unreal
import json
import hashlib
import traceback
from pathlib import Path
from datetime import datetime, timezone

ROOT = Path(r"C:\Users\ndasi\OneDrive\Documents\GitHub\The-Unit")
SOURCE_ROOT = ROOT / "ExternalAssets" / "DonetskHighFidelity"
MANIFEST_PATH = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
RECEIPT_PATH = SOURCE_ROOT / "import_receipt.json"
TRACE_PATH = SOURCE_ROOT / "import_trace.txt"

SOURCE_ROOT.mkdir(parents=True, exist_ok=True)

def trace(message):
    line = f"{datetime.now(timezone.utc).isoformat()} {message}"
    print(line)
    try:
        with TRACE_PATH.open("a", encoding="utf-8") as handle:
            handle.write(line + "\n")
    except Exception:
        pass

def write_receipt(payload):
    RECEIPT_PATH.write_text(
        json.dumps(payload, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

def failure_receipt(error_text, missing=None, imported=None, failed=None, manifest_version=None, required_count=0):
    payload = {
        "schema": "the-unit/donetsk-high-fidelity-import/v1",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "status": "failed",
        "manifest_version": manifest_version,
        "required_count": required_count,
        "imported_count": len(imported or []),
        "missing_count": len(missing or []),
        "failed_count": max(1, len(failed or [])),
        "assets": imported or [],
        "missing": missing or [],
        "failed": failed or [{"target_name": "__bootstrap__", "error": error_text}],
        "fatal_error": error_text,
        "trace_file": str(TRACE_PATH),
    }
    write_receipt(payload)
    return payload

def main():
    TRACE_PATH.write_text("", encoding="utf-8")
    trace("BOOT importer started")
    trace(f"ROOT={ROOT}")
    trace(f"SOURCE_ROOT={SOURCE_ROOT}")
    trace(f"MANIFEST_PATH={MANIFEST_PATH}")
    trace(f"RECEIPT_PATH={RECEIPT_PATH}")

    if not MANIFEST_PATH.exists():
        raise FileNotFoundError(f"Manifest does not exist: {MANIFEST_PATH}")

    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    specs = list(manifest.get("assets", []))
    trace(f"MANIFEST loaded version={manifest.get('version')} assets={len(specs)}")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    if asset_tools is None:
        raise RuntimeError("Unreal AssetToolsHelpers returned no asset tools instance")

    missing = []
    imported = []
    failed = []

    for index, spec in enumerate(specs, start=1):
        target = spec["target_name"]
        src = SOURCE_ROOT / spec["expected_source"]
        trace(f"ASSET {index}/{len(specs)} target={target} source={src}")

        if not src.exists():
            missing.append(str(src))
            trace(f"MISSING {src}")
            continue

        try:
            task = unreal.AssetImportTask()
            task.filename = str(src)
            task.destination_path = spec["destination"]
            task.destination_name = target
            task.automated = True
            task.replace_existing = True
            task.save = True

            trace(f"IMPORT_START {target}")
            asset_tools.import_asset_tasks([task])
            trace(f"IMPORT_RETURN {target} paths={list(task.imported_object_paths)}")

            found_mesh = False
            mesh_paths = []

            for object_path in task.imported_object_paths:
                asset = unreal.EditorAssetLibrary.load_asset(object_path)

                if isinstance(asset, unreal.StaticMesh):
                    found_mesh = True
                    mesh_paths.append(object_path)

                    try:
                        ns = asset.get_editor_property("nanite_settings")
                        ns.enabled = True
                        asset.set_editor_property("nanite_settings", ns)
                        trace(f"NANITE_OK {object_path}")
                    except Exception as exc:
                        trace(f"NANITE_WARN {object_path}: {exc}")

                    try:
                        asset.set_editor_property("light_map_resolution", 256)
                    except Exception as exc:
                        trace(f"LIGHTMAP_WARN {object_path}: {exc}")

                    unreal.EditorAssetLibrary.save_loaded_asset(asset)

            if not found_mesh:
                raise RuntimeError(
                    f"No StaticMesh imported for {src}. "
                    f"Imported object paths: {list(task.imported_object_paths)}"
                )

            digest = hashlib.sha256(src.read_bytes()).hexdigest()
            imported.append({
                "target_name": target,
                "destination": spec["destination"],
                "source_file": spec["expected_source"],
                "source_url": spec.get("source_url"),
                "license": spec.get("license"),
                "sha256": digest,
                "imported_object_paths": list(task.imported_object_paths),
                "static_mesh_paths": mesh_paths,
            })
            trace(f"HF_IMPORT_OK {target}")

        except Exception as exc:
            error_text = f"{type(exc).__name__}: {exc}"
            failed.append({"target_name": target, "error": error_text})
            trace(f"HF_IMPORT_FAIL {target}: {error_text}")
            trace(traceback.format_exc())

    receipt = {
        "schema": "the-unit/donetsk-high-fidelity-import/v1",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "status": "success" if not missing and not failed else "failed",
        "manifest_version": manifest.get("version"),
        "required_count": len(specs),
        "imported_count": len(imported),
        "missing_count": len(missing),
        "failed_count": len(failed),
        "assets": imported,
        "missing": missing,
        "failed": failed,
        "trace_file": str(TRACE_PATH),
    }

    write_receipt(receipt)
    trace(
        f"DONETSK_HIGH_FIDELITY_IMPORT imported={len(imported)} "
        f"missing={len(missing)} failed={len(failed)} receipt={RECEIPT_PATH}"
    )

    if missing or failed:
        unreal.log_error(
            "Donetsk high-fidelity import incomplete. "
            f"missing={len(missing)} failed={len(failed)}"
        )
        return False

    return True

ok = False
try:
    ok = main()
except Exception as exc:
    fatal = f"{type(exc).__name__}: {exc}"
    trace(f"FATAL {fatal}")
    trace(traceback.format_exc())
    try:
        manifest_version = None
        required_count = 0
        if MANIFEST_PATH.exists():
            raw = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
            manifest_version = raw.get("version")
            required_count = len(raw.get("assets", []))
        failure_receipt(
            fatal,
            manifest_version=manifest_version,
            required_count=required_count,
        )
    except Exception as receipt_exc:
        trace(f"FATAL_RECEIPT_WRITE_FAIL {type(receipt_exc).__name__}: {receipt_exc}")
finally:
    trace(f"EXIT success={ok}")
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception as exc:
        trace(f"QUIT_WARN {type(exc).__name__}: {exc}")
