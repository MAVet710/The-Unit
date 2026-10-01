import hashlib
import json
import traceback
from datetime import datetime, timezone
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
LEDGER_PATH = ROOT / "Tools" / "Reference" / "donetsk_surface_source_ledger.json"
RECEIPT_PATH = ROOT / "Saved" / "Automation" / "DonetskVisual" / "material_receipt.json"
MAT_ROOT = "/Game/TheUnit/Donetsk/Materials"

MASTER_PATHS = {
    "surface": f"{MAT_ROOT}/M_Donetsk_Surface_Master",
    "glass": f"{MAT_ROOT}/M_Donetsk_Glass_Master",
    "foliage": f"{MAT_ROOT}/M_Donetsk_Foliage_Master",
}
INSTANCE_NAMES = [
    "Asphalt",
    "Paving",
    "AgedConcrete",
    "CurbConcrete",
    "Plaster",
    "PanelWarm",
    "PanelCool",
    "Brick",
    "GalvanizedMetal",
    "RustedSteel",
    "Wood",
    "Soil",
    "WetGround",
    "Glass",
    "Foliage",
    "VehiclePaint",
]
INSTANCE_KIND = {
    "Glass": "glass",
    "Foliage": "foliage",
}
TEXTURE_PARAMS = {
    "BaseColorTex": "BaseColor",
    "NormalTex": "Normal",
    "RoughnessTex": "Roughness",
    "MetallicTex": "Metallic",
}


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def enum_name(value):
    name = getattr(value, "name", None)
    if name:
        return str(name)
    text = str(value)
    if "." in text:
        text = text.split(".")[-1]
    if ":" in text:
        text = text.split(":")[0]
    return text.strip("<> ")


def asset(path):
    return unreal.EditorAssetLibrary.load_asset(path)


def parameter_names(mat):
    names = []
    for fn in (
        unreal.MaterialEditingLibrary.get_texture_parameter_names,
        unreal.MaterialEditingLibrary.get_scalar_parameter_names,
        unreal.MaterialEditingLibrary.get_vector_parameter_names,
    ):
        try:
            names.extend(str(x) for x in fn(mat))
        except Exception:
            pass
    return sorted(set(names))


def master_record(kind, path):
    mat = asset(path)
    if not isinstance(mat, unreal.Material):
        raise RuntimeError(f"Missing material master: {path}")
    domain = mat.get_editor_property("material_domain")
    blend = mat.get_editor_property("blend_mode")
    dbuffer = (
        domain == unreal.MaterialDomain.MD_SURFACE
        and blend in (unreal.BlendMode.BLEND_OPAQUE, unreal.BlendMode.BLEND_MASKED)
    )
    return {
        "kind": kind,
        "asset_path": path,
        "parameters": parameter_names(mat),
        "material_domain": enum_name(domain),
        "blend_mode": enum_name(blend),
        "two_sided": bool(mat.get_editor_property("two_sided")),
        "dbuffer_compatible": bool(dbuffer),
    }


def texture_record(tex, kind):
    return {
        "asset_path": tex.get_path_name(),
        "kind": kind,
        "srgb": bool(tex.get_editor_property("srgb")),
        "compression": enum_name(tex.get_editor_property("compression_settings")),
    }
def instance_record(name, ledger_by_name):
    path = f"{MAT_ROOT}/MI_Donetsk_{name}"
    mi = asset(path)
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        raise RuntimeError(f"Missing material instance: {path}")

    parent = mi.get_editor_property("parent")
    parent_path = parent.get_path_name() if parent else ""
    expected_kind = INSTANCE_KIND.get(name, "surface")
    expected_parent_asset = asset(MASTER_PATHS[expected_kind])
    expected_parent = (
        expected_parent_asset.get_path_name()
        if expected_parent_asset
        else MASTER_PATHS[expected_kind]
    )
    textures = {}
    texture_records = []
    missing = []
    for param, kind in TEXTURE_PARAMS.items():
        tex = unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(
            mi, param
        )
        if not isinstance(tex, unreal.Texture2D):
            missing.append(param)
            textures[param] = ""
        else:
            textures[param] = tex.get_path_name()
            texture_records.append(texture_record(tex, kind))

    metallic = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(
        mi, "MetallicScale"
    )
    ledger = ledger_by_name.get(name, {})
    return {
        "name": f"MI_Donetsk_{name}",
        "surface_name": name,
        "asset_path": path,
        "parent": parent_path,
        "expected_parent": expected_parent,
        "parent_ok": parent_path == expected_parent,
        "texture_parameters": textures,
        "missing_texture_parameters": missing,
        "physical_surface": ledger.get("physical_surface"),
        "metallic_max": float(metallic),
        "pass": parent_path == expected_parent and not missing,
        "_textures": texture_records,
    }


def validate_ledger(ledger):
    failures = []
    for item in ledger.get("surfaces", []):
        if item.get("approval_state") != "approved":
            failures.append(f"ledger approval: {item.get('name')}")
        if item.get("license") not in ("Project-owned original", "CC0"):
            failures.append(f"ledger license: {item.get('name')}")
        for rel in item.get("local_files", []):
            file_path = ROOT / rel
            if not file_path.exists():
                failures.append(f"ledger file missing: {rel}")
        prefix = item.get("source_prefix")
        hashes = item.get("sha256", {})
        for kind in ("BaseColor", "Normal", "Roughness", "Metallic"):
            file_path = ROOT / "ExternalAssets" / "GeneratedDonetsk" / "Textures" / f"{prefix}_{kind}.png"
            expected = hashes.get(kind)
            if not expected or not file_path.exists() or sha256_file(file_path) != expected:
                failures.append(f"ledger hash mismatch: {item.get('name')}/{kind}")
    return failures


def main():
    RECEIPT_PATH.parent.mkdir(parents=True, exist_ok=True)
    ledger = json.loads(LEDGER_PATH.read_text(encoding="utf-8"))
    ledger_by_name = {item["name"]: item for item in ledger.get("surfaces", [])}
    failures = validate_ledger(ledger)

    masters = []
    for kind, path in MASTER_PATHS.items():
        try:
            record = master_record(kind, path)
            masters.append(record)
        except Exception as exc:
            failures.append(f"{type(exc).__name__}: {exc}")

    instances = []
    texture_map = {}
    for name in INSTANCE_NAMES:
        try:
            record = instance_record(name, ledger_by_name)
            for tex in record.pop("_textures"):
                texture_map[(tex["asset_path"], tex["kind"])] = tex
            if not record["pass"]:
                failures.append(f"instance invalid: {name}")
            instances.append(record)
        except Exception as exc:
            failures.append(f"{type(exc).__name__}: {exc}")

    surface_master = next(
        (x for x in masters if x["kind"] == "surface"),
        None,
    )
    required_params = {
        "BaseColorTex",
        "NormalTex",
        "RoughnessTex",
        "MetallicTex",
        "UVScale",
        "MacroTint",
        "MacroRoughness",
        "DetailNormalStrength",
        "Wetness",
        "Variation",
        "WorldAlignedFallback",
    }
    if not surface_master or not required_params.issubset(set(surface_master["parameters"])):
        failures.append("surface master parameter contract incomplete")
    if surface_master and not surface_master["dbuffer_compatible"]:
        failures.append("surface master is not DBuffer-compatible")

    textures = sorted(
        texture_map.values(),
        key=lambda x: (x["asset_path"], x["kind"]),
    )
    for tex in textures:
        if tex["kind"] == "Normal":
            if tex["srgb"] or tex["compression"] != "TC_NORMALMAP":
                failures.append(f"normal texture settings: {tex['asset_path']}")
        elif tex["kind"] in ("Roughness", "Metallic"):
            if tex["srgb"]:
                failures.append(f"linear mask texture settings: {tex['asset_path']}")

    status = "success" if not failures else "failed"
    payload = {
        "schema": "the-unit/donetsk-material-quality/v1",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "status": status,
        "failed_count": len(failures),
        "masters": masters,
        "instances": instances,
        "textures": textures,
        "failed": failures,
    }
    RECEIPT_PATH.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(
        f"DONETSK_MATERIAL_VALIDATE status={status} "
        f"masters={len(masters)} instances={len(instances)} "
        f"textures={len(textures)} failed={len(failures)}"
    )
    for failure in failures:
        print(f"DONETSK_MATERIAL_VALIDATE_FAIL {failure}")
    return status == "success"


ok = False
try:
    ok = main()
except Exception as exc:
    print(f"DONETSK_MATERIAL_VALIDATE_FATAL {type(exc).__name__}: {exc}")
    print(traceback.format_exc())
finally:
    print(f"DONETSK_MATERIAL_VALIDATE_EXIT success={ok}")
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception:
        pass
