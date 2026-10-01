import hashlib
import json
import traceback
from datetime import datetime, timezone
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = ROOT / "ExternalAssets" / "DonetskHighFidelity"
MANIFEST_PATH = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
RECEIPT_PATH = SOURCE_ROOT / "visual_quality_receipt.json"
SCALE_TOLERANCE = 0.035
DEFAULT_MATERIAL_TOKENS = (
    "/Engine/EngineMaterials/WorldGridMaterial",
    "/Engine/EngineMaterials/DefaultMaterial",
)


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def source_dimensions_cm(mesh):
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


def material_paths(mesh):
    paths = []
    for slot in mesh.get_editor_property("static_materials"):
        interface = slot.get_editor_property("material_interface")
        paths.append(interface.get_path_name() if interface else "")
    return paths


def contains_default_material(paths):
    return any(
        not path or any(token in path for token in DEFAULT_MATERIAL_TOKENS)
        for path in paths
    )
def repair_default_materials(mesh, replacement_path):
    paths = material_paths(mesh)
    if not contains_default_material(paths):
        return False

    material = unreal.EditorAssetLibrary.load_asset(replacement_path)
    if not isinstance(material, unreal.MaterialInterface):
        raise RuntimeError(f"Material override missing: {replacement_path}")

    repaired = False
    for index, path in enumerate(paths):
        if not path or any(token in path for token in DEFAULT_MATERIAL_TOKENS):
            mesh.set_material(index, material)
            repaired = True
    if repaired:
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    return repaired


def ensure_nanite(mesh, required):
    settings = mesh.get_editor_property("nanite_settings")
    enabled = bool(settings.get_editor_property("enabled"))
    repaired = False
    if required and not enabled:
        settings.set_editor_property("enabled", True)
        mesh.set_editor_property("nanite_settings", settings)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        repaired = True
        settings = mesh.get_editor_property("nanite_settings")
        enabled = bool(settings.get_editor_property("enabled"))
    return enabled, repaired


def sourcing_state(spec, passed, source_exists):
    if not source_exists:
        return "community_search"
    if not passed:
        return "generate_required"
    if spec.get("source_type") == "meshy_community":
        return "community_selected"
    return "approved"


def main():
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if subsystem is None:
        raise RuntimeError("StaticMeshEditorSubsystem unavailable")

    unreal.SystemLibrary.execute_console_command(
        None, "Editor.AsyncStaticMeshCompilationFinishAll"
    )
    results = []
    failures = []
    queue = []

    for spec in manifest.get("assets", []):
        name = spec["target_name"]
        asset_path = f'{spec["destination"]}/{name}.{name}'
        source_file = SOURCE_ROOT / spec["expected_source"]
        result = {
            "target_name": name,
            "asset_path": asset_path,
            "role": spec.get("role"),
            "source_type": spec.get("source_type"),
            "source_url": spec.get("source_url"),
            "license": spec.get("license"),
            "source_file": str(source_file),
            "source_sha256": sha256_file(source_file) if source_file.exists() else "",
            "manual_review": "pending",
            "nanite_required": bool(spec.get("nanite_required", False)),
            "pass": False,
        }

        try:
            mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
            if not isinstance(mesh, unreal.StaticMesh):
                raise RuntimeError(f"StaticMesh not found: {asset_path}")

            material_repaired = repair_default_materials(
                mesh, spec.get("material_override", "")
            )
            nanite_enabled, nanite_repaired = ensure_nanite(
                mesh, result["nanite_required"]
            )
            unreal.SystemLibrary.execute_console_command(
                None, "Editor.AsyncStaticMeshCompilationFinishAll"
            )
            paths = material_paths(mesh)
            bounds = source_dimensions_cm(mesh)
            measured = measured_dimension(bounds, spec["scale_mode"])
            settings = subsystem.get_lod_build_settings(mesh, 0)
            build_scale3d = settings.get_editor_property("build_scale3d")
            build_scale = {
                "x": float(build_scale3d.x),
                "y": float(build_scale3d.y),
                "z": float(build_scale3d.z),
            }
            build_scale_ok = all(
                abs(value - 1.0) <= 0.001
                for value in build_scale.values()
            )
            target = float(spec["target_cm"])
            relative_error = abs(measured - target) / target
            default_material = contains_default_material(paths)
            checks = {
                "source_exists": source_file.exists(),
                "positive_bounds": all(value > 0.0 for value in bounds.values()),
                "has_material_slot": len(paths) >= 1,
                "no_default_material": not default_material,
                "scale_in_tolerance": relative_error <= SCALE_TOLERANCE,
                "build_scale_is_one": build_scale_ok,
                "nanite_ok": (not result["nanite_required"]) or nanite_enabled,
            }
            passed = all(checks.values())
            result.update({
                "build_scale3d": build_scale,
                "bounds_cm": bounds,
                "scale_mode": spec["scale_mode"],
                "target_cm": target,
                "measured_cm": measured,
                "relative_scale_error": relative_error,
                "material_slot_count": len(paths),
                "material_paths": paths,
                "default_material_detected": default_material,
                "material_repaired": material_repaired,
                "nanite_enabled": nanite_enabled,
                "nanite_repaired": nanite_repaired,
                "checks": checks,
                "pass": passed,
            })
            if not passed:
                failures.append({
                    "target_name": name,
                    "failed_checks": [key for key, value in checks.items() if not value],
                })
        except Exception as exc:
            result["error"] = f"{type(exc).__name__}: {exc}"
            failures.append({"target_name": name, "error": result["error"]})
            unreal.log_error(traceback.format_exc())

        results.append(result)
        state = sourcing_state(spec, result["pass"], source_file.exists())
        queue.append({
            "target_name": name,
            "role": spec.get("role"),
            "status": state,
            "source_type": spec.get("source_type"),
            "source_url": spec.get("source_url"),
            "license": spec.get("license"),
            "meshy_task_id": spec.get("meshy_task_id"),
        })

    status = (
        "success"
        if not failures and len(results) == len(manifest.get("assets", []))
        else "failed"
    )
    payload = {
        "schema": "the-unit/donetsk-visual-quality/v1",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "manifest_version": manifest.get("version"),
        "required_count": len(manifest.get("assets", [])),
        "validated_count": len(results),
        "failed_count": len(failures),
        "status": status,
        "manual_review": "pending",
        "assets": results,
        "sourcing_queue": queue,
        "failed": failures,
    }
    RECEIPT_PATH.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    unreal.log(f"DONETSK_VISUAL_QUALITY status={status} failed={len(failures)}")
    return status == "success"
ok = False
try:
    ok = main()
except Exception as exc:
    unreal.log_error(f"DONETSK_VISUAL_QUALITY_FATAL {type(exc).__name__}: {exc}")
    unreal.log_error(traceback.format_exc())
finally:
    unreal.log(f"DONETSK_VISUAL_QUALITY_EXIT success={ok}")
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception:
        pass
