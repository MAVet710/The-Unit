import json
import traceback
from datetime import datetime, timezone
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = ROOT / "ExternalAssets" / "DonetskHighFidelity"
MANIFEST_PATH = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
RECEIPT_PATH = SOURCE_ROOT / "scale_receipt.json"
TRACE_PATH = SOURCE_ROOT / "scale_trace.txt"
TOLERANCE = 0.035
BUILD_SCALE_TOLERANCE = 0.001


def trace(message):
    line = f"{datetime.now(timezone.utc).isoformat()} {message}"
    print(line)
    with TRACE_PATH.open("a", encoding="utf-8") as handle:
        handle.write(line + "\n")


def dimensions_cm(mesh):
    box = mesh.get_bounding_box()
    size = box.max - box.min
    return {
        "x": abs(float(size.x)),
        "y": abs(float(size.y)),
        "z": abs(float(size.z)),
    }


def measured_dimension(bounds, mode):
    if mode == "height_cm":
        return bounds["z"]
    if mode == "longest_cm":
        return max(bounds.values())
    raise ValueError(f"Unknown scale mode: {mode}")


def main():
    TRACE_PATH.write_text("", encoding="utf-8")
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    specs = list(manifest.get("assets", []))
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if subsystem is None:
        raise RuntimeError("StaticMeshEditorSubsystem unavailable")

    unreal.SystemLibrary.execute_console_command(
        None, "Editor.AsyncStaticMeshCompilationFinishAll"
    )

    results = []
    failures = []

    for spec in specs:
        name = spec["target_name"]
        asset_path = f'{spec["destination"]}/{name}.{name}'
        try:
            mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
            if not isinstance(mesh, unreal.StaticMesh):
                raise RuntimeError(f"StaticMesh not found: {asset_path}")

            unreal.SystemLibrary.execute_console_command(
                None, "Editor.AsyncStaticMeshCompilationFinishAll"
            )
            bounds = dimensions_cm(mesh)
            measured = measured_dimension(bounds, spec["scale_mode"])
            target = float(spec["target_cm"])
            relative_error = abs(measured - target) / target

            settings = subsystem.get_lod_build_settings(mesh, 0)
            scale = settings.get_editor_property("build_scale3d")
            build_scale = {
                "x": float(scale.x),
                "y": float(scale.y),
                "z": float(scale.z),
            }
            build_scale_ok = all(
                abs(value - 1.0) <= BUILD_SCALE_TOLERANCE
                for value in build_scale.values()
            )
            passed = relative_error <= TOLERANCE and build_scale_ok
            result = {
                "target_name": name,
                "asset_path": asset_path,
                "scale_mode": spec["scale_mode"],
                "target_cm": target,
                "bounds_cm": bounds,
                "measured_after_cm": measured,
                "relative_error": relative_error,
                "build_scale3d": build_scale,
                "pass": passed,
            }
            results.append(result)
            trace(
                f"VERIFY {name} measured={measured:.2f} "
                f"error={relative_error:.4%} build_scale={build_scale} pass={passed}"
            )
            if not passed:
                failures.append({
                    "target_name": name,
                    "relative_error": relative_error,
                    "build_scale3d": build_scale,
                })
        except Exception as exc:
            failures.append({
                "target_name": name,
                "error": f"{type(exc).__name__}: {exc}",
            })
            trace(f"FAIL {name}: {type(exc).__name__}: {exc}")
            trace(traceback.format_exc())

    status = (
        "success"
        if not failures and len(results) == len(specs)
        else "failed"
    )
    payload = {
        "schema": "the-unit/donetsk-high-fidelity-scale/v3",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "manifest_version": manifest.get("version"),
        "required_count": len(specs),
        "validated_count": len(results),
        "failed_count": len(failures),
        "status": status,
        "tolerance": TOLERANCE,
        "build_scale_tolerance": BUILD_SCALE_TOLERANCE,
        "assets": results,
        "failed": failures,
        "trace_file": str(TRACE_PATH),
    }
    RECEIPT_PATH.write_text(
        json.dumps(payload, indent=2) + "\n",
        encoding="utf-8",
    )
    trace(f"DONE status={status} failed={len(failures)}")
    return status == "success"


ok = False
try:
    ok = main()
except Exception as exc:
    trace(f"FATAL {type(exc).__name__}: {exc}")
    trace(traceback.format_exc())
finally:
    trace(f"EXIT success={ok}")
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception:
        pass
