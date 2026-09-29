import json
import traceback
from pathlib import Path
from datetime import datetime, timezone

import unreal

ROOT = Path(r"C:\Users\ndasi\OneDrive\Documents\GitHub\The-Unit")
SOURCE_ROOT = ROOT / "ExternalAssets" / "DonetskHighFidelity"
MANIFEST_PATH = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
RECEIPT_PATH = SOURCE_ROOT / "scale_receipt.json"
TRACE_PATH = SOURCE_ROOT / "scale_trace.txt"

TOLERANCE = 0.035


def trace(message):
    line = f"{datetime.now(timezone.utc).isoformat()} {message}"
    print(line)
    with TRACE_PATH.open("a", encoding="utf-8") as handle:
        handle.write(line + "\n")


def get_dimensions_cm(mesh):
    # UStaticMesh.get_bounding_box() returns unreal.Box. In UE 5.7 Python,
    # use the reflected min/max vectors directly rather than a C++ helper method.
    box = mesh.get_bounding_box()
    return box.max - box.min


def select_measure(dimensions, mode):
    x = abs(float(dimensions.x))
    y = abs(float(dimensions.y))
    z = abs(float(dimensions.z))
    if mode == "height_cm":
        return z
    if mode == "longest_cm":
        return max(x, y, z)
    raise ValueError(f"Unknown scale_mode: {mode}")


def vector_dict(v):
    return {"x": float(v.x), "y": float(v.y), "z": float(v.z)}


def main():
    TRACE_PATH.write_text("", encoding="utf-8")
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

    if subsystem is None:
        raise RuntimeError("StaticMeshEditorSubsystem unavailable")

    results = []
    failures = []

    for spec in manifest.get("assets", []):
        target_name = spec["target_name"]
        asset_path = f'{spec["destination"]}/{target_name}.{target_name}'
        mode = spec.get("scale_mode")
        target_cm = float(spec.get("target_cm", 0.0))

        if not mode or target_cm <= 0.0:
            failures.append({"target_name": target_name, "error": "missing scale target"})
            continue

        mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(mesh, unreal.StaticMesh):
            failures.append({"target_name": target_name, "error": f"StaticMesh not found: {asset_path}"})
            continue

        try:
            before = get_dimensions_cm(mesh)
            measured_before = select_measure(before, mode)
            if measured_before <= 0.001:
                raise RuntimeError(f"invalid measured dimension {measured_before}")

            lod_count = max(1, int(subsystem.get_lod_count(mesh)))
            cumulative_factor = 1.0
            passes = []

            for pass_index in range(1, 5):
                current = get_dimensions_cm(mesh)
                measured_current = select_measure(current, mode)
                if measured_current <= 0.001:
                    raise RuntimeError(f"invalid measured dimension {measured_current}")

                relative_error_current = abs(measured_current - target_cm) / target_cm
                passes.append({
                    "pass": pass_index,
                    "dimensions_cm": vector_dict(current),
                    "measured_cm": measured_current,
                    "relative_error": relative_error_current,
                })

                if relative_error_current <= TOLERANCE:
                    break

                factor = target_cm / measured_current
                cumulative_factor *= factor

                trace(
                    f"SCALE_PASS {target_name} pass={pass_index} mode={mode} "
                    f"target={target_cm:.2f} measured={measured_current:.2f} factor={factor:.6f}"
                )

                for lod_index in range(lod_count):
                    settings = subsystem.get_lod_build_settings(mesh, lod_index)
                    old_scale = settings.get_editor_property("build_scale3d")
                    settings.set_editor_property(
                        "build_scale3d",
                        unreal.Vector(
                            float(old_scale.x) * factor,
                            float(old_scale.y) * factor,
                            float(old_scale.z) * factor,
                        ),
                    )
                    subsystem.set_lod_build_settings(mesh, lod_index, settings)

                unreal.EditorAssetLibrary.save_loaded_asset(mesh)

            after = get_dimensions_cm(mesh)
            measured_after = select_measure(after, mode)
            relative_error = abs(measured_after - target_cm) / target_cm
            passed = relative_error <= TOLERANCE

            trace(
                f"RESULT {target_name} after=({after.x:.2f},{after.y:.2f},{after.z:.2f}) "
                f"measured={measured_after:.2f} error={relative_error:.4%} "
                f"cumulative_factor={cumulative_factor:.6f} pass={passed}"
            )

            result = {
                "target_name": target_name,
                "asset_path": asset_path,
                "scale_mode": mode,
                "target_cm": target_cm,
                "before_dimensions_cm": vector_dict(before),
                "applied_factor": cumulative_factor,
                "passes": passes,
                "after_dimensions_cm": vector_dict(after),
                "measured_after_cm": measured_after,
                "relative_error": relative_error,
                "pass": passed,
            }
            results.append(result)
            if not passed:
                failures.append({
                    "target_name": target_name,
                    "error": f"post-scale dimension {measured_after:.2f} cm outside tolerance for target {target_cm:.2f} cm",
                })

        except Exception as exc:
            failures.append({"target_name": target_name, "error": f"{type(exc).__name__}: {exc}"})
            trace(f"FAIL {target_name}: {type(exc).__name__}: {exc}")
            trace(traceback.format_exc())

    payload = {
        "schema": "the-unit/donetsk-high-fidelity-scale/v1",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "manifest_version": manifest.get("version"),
        "required_count": len(manifest.get("assets", [])),
        "scaled_count": len(results),
        "failed_count": len(failures),
        "status": "success" if not failures and len(results) == len(manifest.get("assets", [])) else "failed",
        "tolerance": TOLERANCE,
        "assets": results,
        "failed": failures,
        "trace_file": str(TRACE_PATH),
    }
    RECEIPT_PATH.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    trace(f"DONE scaled={len(results)} failed={len(failures)} receipt={RECEIPT_PATH}")
    return not failures


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
