import json
import traceback
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = ROOT / "Tools" / "AssetPrep" / "donetsk_high_fidelity_assets.json"


def main():
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    applied = []
    failures = []

    for spec in manifest.get("assets", []):
        name = spec["target_name"]
        mesh_path = f'{spec["destination"]}/{name}.{name}'
        material_path = spec["material_override"]
        try:
            mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
            material = unreal.EditorAssetLibrary.load_asset(material_path)
            if not isinstance(mesh, unreal.StaticMesh):
                raise RuntimeError(f"StaticMesh missing: {mesh_path}")
            if not isinstance(material, unreal.MaterialInterface):
                raise RuntimeError(f"Material missing: {material_path}")

            slots = list(mesh.get_editor_property("static_materials"))
            if not slots:
                raise RuntimeError(f"StaticMesh has no material slots: {mesh_path}")

            for index in range(len(slots)):
                mesh.set_material(index, material)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)

            resolved = []
            for index in range(len(slots)):
                bound = mesh.get_material(index)
                resolved.append(bound.get_path_name() if bound else "")
            if not resolved or any(path != material.get_path_name() for path in resolved):
                raise RuntimeError(
                    f"Material binding did not persist: expected={material.get_path_name()} "
                    f"actual={resolved}"
                )

            applied.append({
                "target_name": name,
                "asset_path": mesh_path,
                "material_path": material.get_path_name(),
                "slot_count": len(slots),
            })
            print(
                f"DONETSK_MATERIAL_APPLY {name} "
                f"slots={len(slots)} material={material.get_path_name()}"
            )
        except Exception as exc:
            failure = f"{type(exc).__name__}: {exc}"
            failures.append({"target_name": name, "error": failure})
            print(f"DONETSK_MATERIAL_APPLY_FAIL {name} {failure}")
            print(traceback.format_exc())

    status = "success" if not failures and len(applied) == len(manifest.get("assets", [])) else "failed"
    print(
        f"DONETSK_MATERIAL_APPLY_DONE status={status} "
        f"applied={len(applied)} failed={len(failures)}"
    )
    return status == "success"


ok = False
try:
    ok = main()
except Exception as exc:
    print(f"DONETSK_MATERIAL_APPLY_FATAL {type(exc).__name__}: {exc}")
    print(traceback.format_exc())
finally:
    print(f"DONETSK_MATERIAL_APPLY_EXIT success={ok}")
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception:
        pass
