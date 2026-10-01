import json
import traceback
from pathlib import Path
from datetime import datetime, timezone

import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = ROOT / "ExternalAssets" / "DonetskHighFidelity"
MANIFEST_PATH = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
RECEIPT_PATH = SOURCE_ROOT / "scale_receipt.json"
TRACE_PATH = SOURCE_ROOT / "scale_trace.txt"

TOLERANCE = 0.035
IDEMPOTENCE_TOLERANCE = TOLERANCE


def trace(message):
    line = f"{datetime.now(timezone.utc).isoformat()} {message}"
    print(line)
    with TRACE_PATH.open("a", encoding="utf-8") as handle:
        handle.write(line + "\n")


def get_dimensions_cm(mesh):
    box = mesh.get_bounding_box()
    return box.max - box.min


def select_measure(dimensions, mode):
    values = (
        abs(float(dimensions.x)),
        abs(float(dimensions.y)),
        abs(float(dimensions.z)),
    )
    if mode == "height_cm":
        return values[2]
    if mode == "longest_cm":
        return max(values)
    raise ValueError(f"Unknown scale_mode: {mode}")
def vector_dict(v):
    return {"x": float(v.x), "y": float(v.y), "z": float(v.z)}


def scale_dict(v):
    return {"x": float(v.x), "y": float(v.y), "z": float(v.z)}


def get_build_scales(mesh, subsystem):
    count = max(1, int(subsystem.get_lod_count(mesh)))
    return [
        scale_dict(
            subsystem.get_lod_build_settings(mesh, i).get_editor_property(
                "build_scale3d"
            )
        )
        for i in range(count)
    ]


def finish_static_mesh_compilation():
    unreal.SystemLibrary.execute_console_command(
        None, "Editor.AsyncStaticMeshCompilationFinishAll"
    )


def force_synchronous_static_mesh_builds():
    finish_static_mesh_compilation()
    unreal.SystemLibrary.execute_console_command(
        None, "Editor.AsyncStaticMeshCompilation 0"
    )
    finish_static_mesh_compilation()


def apply_scale_factor(mesh, subsystem, factor):
    count = max(1, int(subsystem.get_lod_count(mesh)))
    for lod_index in range(count):
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
    finish_static_mesh_compilation()
def main():
    TRACE_PATH.write_text("", encoding="utf-8")
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    specs = list(manifest.get("assets", []))
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if subsystem is None:
        raise RuntimeError("StaticMeshEditorSubsystem unavailable")

    force_synchronous_static_mesh_builds()

    results = []
    failures = []
    corrected = []

    for spec in specs:
        target_name = spec["target_name"]
        asset_path = f'{spec["destination"]}/{target_name}.{target_name}'
        mode = spec.get("scale_mode")
        target_cm = float(spec.get("target_cm", 0.0))

        if not mode or target_cm <= 0.0:
            failures.append(
                {"target_name": target_name, "error": "missing scale target"}
            )
            continue

        mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(mesh, unreal.StaticMesh):
            failures.append(
                {"target_name": target_name, "error": f"StaticMesh not found: {asset_path}"}
            )
            continue

        try:
            before = get_dimensions_cm(mesh)
            measured_before = select_measure(before, mode)
            if measured_before <= 0.001:
                raise RuntimeError(f"invalid measured dimension {measured_before}")

            factor = target_cm / measured_before
            relative_error = abs(measured_before - target_cm) / target_cm
            build_scale_before = get_build_scales(mesh, subsystem)
            needs_correction = abs(factor - 1.0) > IDEMPOTENCE_TOLERANCE
            if needs_correction:
                trace(
                    f"CORRECT {target_name} mode={mode} target={target_cm:.2f} "
                    f"measured={measured_before:.2f} factor={factor:.6f}"
                )
                apply_scale_factor(mesh, subsystem, factor)
                build_scale_after = get_build_scales(mesh, subsystem)
                corrected.append(target_name)
                result = {
                    "target_name": target_name,
                    "asset_path": asset_path,
                    "scale_mode": mode,
                    "target_cm": target_cm,
                    "before_dimensions_cm": vector_dict(before),
                    "build_scale_before": build_scale_before,
                    "iterations": 1,
                    "after_dimensions_cm": None,
                    "build_scale_after": build_scale_after,
                    "measured_after_cm": None,
                    "relative_error": relative_error,
                    "idempotence_factor": factor,
                    "pass": None,
                    "verification_required": True,
                }
                trace(
                    f"PENDING {target_name} correction saved; "
                    "fresh editor verification required"
                )
            else:
                passed = relative_error <= TOLERANCE
                result = {
                    "target_name": target_name,
                    "asset_path": asset_path,
                    "scale_mode": mode,
                    "target_cm": target_cm,
                    "before_dimensions_cm": vector_dict(before),
                    "build_scale_before": build_scale_before,
                    "iterations": 0,
                    "after_dimensions_cm": vector_dict(before),
                    "build_scale_after": build_scale_before,
                    "measured_after_cm": measured_before,
                    "relative_error": relative_error,
                    "idempotence_factor": factor,
                    "pass": passed,
                    "verification_required": False,
                }
                trace(
                    f"VERIFY {target_name} measured={measured_before:.2f} "
                    f"error={relative_error:.4%} idempotence={factor:.6f} pass={passed}"
                )
                if not passed:
                    failures.append({
                        "target_name": target_name,
                        "error": (
                            f"dimension {measured_before:.2f} cm outside tolerance "
                            f"for target {target_cm:.2f} cm"
                        ),
                    })
            results.append(result)
        except Exception as exc:
            failures.append(
                {"target_name": target_name, "error": f"{type(exc).__name__}: {exc}"}
            )
            trace(f"FAIL {target_name}: {type(exc).__name__}: {exc}")
            trace(traceback.format_exc())

    if failures:
        status = "failed"
    elif corrected:
        status = "needs_verification"
    elif len(results) == len(specs):
        status = "success"
    else:
        status = "failed"

    payload = {
        "schema": "the-unit/donetsk-high-fidelity-scale/v2",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "manifest_version": manifest.get("version"),
        "required_count": len(specs),
        "scaled_count": len(results),
        "correction_count": len(corrected),
        "corrected_assets": corrected,
        "failed_count": len(failures),
        "status": status,
        "tolerance": TOLERANCE,
        "idempotence_tolerance": IDEMPOTENCE_TOLERANCE,
        "assets": results,
        "failed": failures,
        "trace_file": str(TRACE_PATH),
    }
    RECEIPT_PATH.write_text(
        json.dumps(payload, indent=2) + "\n",
        encoding="utf-8",
    )
    trace(
        f"DONE status={status} assets={len(results)} corrected={len(corrected)} "
        f"failed={len(failures)} receipt={RECEIPT_PATH}"
    )
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
