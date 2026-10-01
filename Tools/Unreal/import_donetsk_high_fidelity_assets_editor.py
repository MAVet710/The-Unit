import hashlib
import json
import traceback
from datetime import datetime, timezone
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = ROOT / "ExternalAssets" / "DonetskHighFidelity"
MANIFEST_PATH = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"
RECEIPT_PATH = SOURCE_ROOT / "import_receipt.json"
TRACE_PATH = SOURCE_ROOT / "import_trace.txt"
SCALE_TOLERANCE = 0.035

SOURCE_ROOT.mkdir(parents=True, exist_ok=True)


def trace(message):
    line = f"{datetime.now(timezone.utc).isoformat()} {message}"
    print(line)
    try:
        with TRACE_PATH.open("a", encoding="utf-8") as handle:
            handle.write(line + "\n")
    except Exception:
        pass


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


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
def make_fbx_options(import_uniform_scale):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property(
        "mesh_type_to_import",
        unreal.FBXImportType.FBXIT_STATIC_MESH,
    )
    for name, value in (("import_materials", False), ("import_textures", False)):
        try:
            ui.set_editor_property(name, value)
        except Exception as exc:
            trace(f"FBX_OPTION_WARN {name}: {type(exc).__name__}: {exc}")

    data = ui.get_editor_property("static_mesh_import_data")
    data.set_editor_property("import_uniform_scale", float(import_uniform_scale))
    try:
        data.set_editor_property("combine_meshes", True)
    except Exception as exc:
        trace(f"FBX_OPTION_WARN combine_meshes: {type(exc).__name__}: {exc}")
    ui.set_editor_property("static_mesh_import_data", data)
    return ui


def reset_build_scale(mesh, subsystem):
    changed = False
    lod_count = max(1, int(subsystem.get_lod_count(mesh)))
    for lod_index in range(lod_count):
        settings = subsystem.get_lod_build_settings(mesh, lod_index)
        scale = settings.get_editor_property("build_scale3d")
        if (
            abs(float(scale.x) - 1.0) > 0.0001
            or abs(float(scale.y) - 1.0) > 0.0001
            or abs(float(scale.z) - 1.0) > 0.0001
        ):
            settings.set_editor_property(
                "build_scale3d",
                unreal.Vector(1.0, 1.0, 1.0),
            )
            subsystem.set_lod_build_settings(mesh, lod_index, settings)
            changed = True
    return changed


def assign_material(mesh, material_path):
    material = unreal.EditorAssetLibrary.load_asset(material_path)
    if not isinstance(material, unreal.MaterialInterface):
        raise RuntimeError(f"Material override missing: {material_path}")

    slots = list(mesh.get_editor_property("static_materials"))
    slot_count = max(1, len(slots))
    for index in range(slot_count):
        mesh.set_material(index, material)


def configure_mesh(mesh, spec, subsystem):
    reset = reset_build_scale(mesh, subsystem)

    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", bool(spec.get("nanite_required", False)))
    mesh.set_editor_property("nanite_settings", nanite)

    try:
        mesh.set_editor_property("light_map_resolution", 256)
    except Exception as exc:
        trace(f"LIGHTMAP_WARN {spec['target_name']}: {type(exc).__name__}: {exc}")

    assign_material(mesh, spec["material_override"])
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.SystemLibrary.execute_console_command(
        None, "Editor.AsyncStaticMeshCompilationFinishAll"
    )
    return reset
def main():
    TRACE_PATH.write_text("", encoding="utf-8")
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    specs = list(manifest.get("assets", []))
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if asset_tools is None or subsystem is None:
        raise RuntimeError("Required Unreal editor subsystem unavailable")

    unreal.SystemLibrary.execute_console_command(
        None, "Editor.AsyncStaticMeshCompilationFinishAll"
    )
    unreal.SystemLibrary.execute_console_command(
        None, "Editor.AsyncStaticMeshCompilation 0"
    )

    imported = []
    missing = []
    failed = []

    for index, spec in enumerate(specs, start=1):
        target = spec["target_name"]
        source = SOURCE_ROOT / spec["expected_source"]
        trace(f"ASSET {index}/{len(specs)} target={target}")

        if not source.exists():
            missing.append(str(source))
            continue

        try:
            scale = float(spec["import_uniform_scale"])
            options = make_fbx_options(scale)

            task = unreal.AssetImportTask()
            task.filename = str(source)
            task.destination_path = spec["destination"]
            task.destination_name = target
            task.automated = True
            task.replace_existing = True
            task.save = True
            task.options = options

            trace(f"IMPORT_START {target} uniform_scale={scale:.6f}")
            asset_tools.import_asset_tasks([task])
            unreal.SystemLibrary.execute_console_command(
                None, "Editor.AsyncStaticMeshCompilationFinishAll"
            )

            asset_path = f'{spec["destination"]}/{target}'
            mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
            if not isinstance(mesh, unreal.StaticMesh):
                raise RuntimeError(
                    f"StaticMesh not found after import: {asset_path}; "
                    f"paths={list(task.imported_object_paths)}"
                )

            reset = configure_mesh(mesh, spec, subsystem)
            bounds = dimensions_cm(mesh)
            measured = measured_dimension(bounds, spec["scale_mode"])
            target_cm = float(spec["target_cm"])
            relative_error = abs(measured - target_cm) / target_cm

            settings = subsystem.get_lod_build_settings(mesh, 0)
            build_scale = settings.get_editor_property("build_scale3d")
            build_scale_dict = {
                "x": float(build_scale.x),
                "y": float(build_scale.y),
                "z": float(build_scale.z),
            }
            scale_is_one = all(
                abs(value - 1.0) <= 0.001
                for value in build_scale_dict.values()
            )
            passed = relative_error <= SCALE_TOLERANCE and scale_is_one
            if not passed:
                raise RuntimeError(
                    f"post-import scale gate failed measured={measured:.2f} "
                    f"target={target_cm:.2f} error={relative_error:.4%} "
                    f"build_scale={build_scale_dict}"
                )

            imported.append({
                "target_name": target,
                "asset_path": asset_path,
                "source_file": spec["expected_source"],
                "source_url": spec.get("source_url"),
                "license": spec.get("license"),
                "source_sha256": sha256_file(source),
                "import_uniform_scale": scale,
                "build_scale_reset": reset,
                "build_scale3d": build_scale_dict,
                "bounds_cm": bounds,
                "scale_mode": spec["scale_mode"],
                "target_cm": target_cm,
                "measured_cm": measured,
                "relative_error": relative_error,
                "pass": True,
            })
            trace(
                f"IMPORT_OK {target} measured={measured:.2f} "
                f"error={relative_error:.4%}"
            )

        except Exception as exc:
            error = f"{type(exc).__name__}: {exc}"
            failed.append({"target_name": target, "error": error})
            trace(f"IMPORT_FAIL {target}: {error}")
            trace(traceback.format_exc())
    status = (
        "success"
        if not missing and not failed and len(imported) == len(specs)
        else "failed"
    )
    payload = {
        "schema": "the-unit/donetsk-high-fidelity-import/v2",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "status": status,
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
    RECEIPT_PATH.write_text(
        json.dumps(payload, indent=2) + "\n",
        encoding="utf-8",
    )
    trace(
        f"DONE status={status} imported={len(imported)} "
        f"missing={len(missing)} failed={len(failed)}"
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
