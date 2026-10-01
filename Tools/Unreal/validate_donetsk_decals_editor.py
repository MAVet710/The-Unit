import hashlib
import json
import traceback
from datetime import datetime, timezone
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
LEDGER_PATH = ROOT / "Tools" / "Reference" / "donetsk_decal_source_ledger.json"
RECEIPT_PATH = ROOT / "Saved" / "Automation" / "DonetskVisual" / "decal_receipt.json"
DEST = "/Game/TheUnit/Donetsk/Decals"
MASTER_PATH = f"{DEST}/M_Donetsk_Decal_Master"
TEXTURE_PARAMS = {
    "BaseColorTex": "BaseColor",
    "NormalTex": "Normal",
    "RoughnessTex": "Roughness",
    "OpacityTex": "Opacity",
}


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


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def asset(path):
    return unreal.EditorAssetLibrary.load_asset(path)


def parameter_names(mat):
    names = []
    for fn in (
        unreal.MaterialEditingLibrary.get_texture_parameter_names,
        unreal.MaterialEditingLibrary.get_scalar_parameter_names,
        unreal.MaterialEditingLibrary.get_vector_parameter_names,
    ):
        names.extend(str(x) for x in fn(mat))
    return sorted(set(names))


def validate_ledger(ledger):
    failures=[]
    for entry in ledger.get("decals", []):
        if entry.get("license") != "Project-owned original":
            failures.append(f"license: {entry.get('name')}")
        if entry.get("approval_state") != "approved":
            failures.append(f"approval: {entry.get('name')}")
        size=entry.get("physical_size_cm",{})
        if not (0 < float(size.get("x",0)) <= 800 and 0 < float(size.get("y",0)) <= 800):
            failures.append(f"physical size: {entry.get('name')}")
        for rel in entry.get("local_files", []):
            path=ROOT/rel
            if not path.exists():
                failures.append(f"missing source: {rel}")
        hashes=entry.get("sha256",{})
        for kind in ("BaseColor","Normal","Roughness","Opacity"):
            path=ROOT/"ExternalAssets"/"DonetskDecals"/f"{entry['name']}_{kind}.png"
            if not path.exists() or hashes.get(kind) != sha256_file(path):
                failures.append(f"hash mismatch: {entry['name']}/{kind}")
    return failures


def texture_record(tex, kind):
    return {
        "asset_path": tex.get_path_name(),
        "kind": kind,
        "srgb": bool(tex.get_editor_property("srgb")),
        "compression": enum_name(tex.get_editor_property("compression_settings")),
    }
def instance_record(entry, expected_parent):
    name=entry["instance_name"]
    path=f"{DEST}/{name}"
    mi=asset(path)
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        raise RuntimeError(f"Missing material instance: {path}")
    parent=mi.get_editor_property("parent")
    parent_path=parent.get_path_name() if parent else ""
    textures={}
    texture_records=[]
    missing=[]
    for param,kind in TEXTURE_PARAMS.items():
        tex=unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,param)
        if not isinstance(tex,unreal.Texture2D):
            missing.append(param)
            textures[param]=""
        else:
            textures[param]=tex.get_path_name()
            texture_records.append(texture_record(tex,kind))
    passed=parent_path==expected_parent and not missing
    return {
        "name":name,
        "asset_path":path,
        "category":entry["category"],
        "physical_size_cm":entry["physical_size_cm"],
        "parent":parent_path,
        "expected_parent":expected_parent,
        "texture_parameters":textures,
        "missing_texture_parameters":missing,
        "pass":passed,
        "_textures":texture_records,
    }


def main():
    RECEIPT_PATH.parent.mkdir(parents=True,exist_ok=True)
    ledger=json.loads(LEDGER_PATH.read_text(encoding="utf-8"))
    failures=validate_ledger(ledger)

    master=asset(MASTER_PATH)
    if not isinstance(master,unreal.Material):
        raise RuntimeError(f"Missing decal master: {MASTER_PATH}")
    domain=master.get_editor_property("material_domain")
    blend=master.get_editor_property("blend_mode")
    params=parameter_names(master)
    required={
        "BaseColorTex","NormalTex","RoughnessTex","OpacityTex",
        "Tint","RoughnessScale","NormalStrength","OpacityScale",
    }
    dbuffer=(
        domain==unreal.MaterialDomain.MD_DEFERRED_DECAL
        and blend==unreal.BlendMode.BLEND_TRANSLUCENT
        and required.issubset(set(params))
    )
    master_record={
        "asset_path":MASTER_PATH,
        "resolved_path":master.get_path_name(),
        "material_domain":enum_name(domain),
        "blend_mode":enum_name(blend),
        "parameters":params,
        "dbuffer_compatible":bool(dbuffer),
    }
    if not dbuffer:
        failures.append("decal master contract")

    expected_parent=master.get_path_name()
    instances=[]
    texture_map={}
    for entry in ledger.get("decals",[]):
        try:
            record=instance_record(entry,expected_parent)
            for tex in record.pop("_textures"):
                texture_map[(tex["asset_path"],tex["kind"])]=tex
            if not record["pass"]:
                failures.append(f"instance invalid: {entry['instance_name']}")
            instances.append(record)
        except Exception as exc:
            failures.append(f"{type(exc).__name__}: {exc}")

    textures=sorted(texture_map.values(),key=lambda x:(x["asset_path"],x["kind"]))
    for tex in textures:
        if tex["kind"]=="Normal":
            if tex["srgb"] or tex["compression"]!="TC_NORMALMAP":
                failures.append(f"normal settings: {tex['asset_path']}")
        elif tex["kind"] in ("Roughness","Opacity"):
            if tex["srgb"]:
                failures.append(f"linear mask settings: {tex['asset_path']}")

    status="success" if not failures and len(instances)>=12 else "failed"
    payload={
        "schema":"the-unit/donetsk-decal-quality/v1",
        "generated_at_utc":datetime.now(timezone.utc).isoformat(),
        "status":status,
        "failed_count":len(failures),
        "master":master_record,
        "instances":instances,
        "textures":textures,
        "failed":failures,
    }
    RECEIPT_PATH.write_text(json.dumps(payload,indent=2)+"\n",encoding="utf-8")
    print(
        f"DONETSK_DECAL_VALIDATE status={status} instances={len(instances)} "
        f"textures={len(textures)} failed={len(failures)}"
    )
    for failure in failures:
        print(f"DONETSK_DECAL_VALIDATE_FAIL {failure}")
    return status=="success"


ok=False
try:
    ok=main()
except Exception as exc:
    print(f"DONETSK_DECAL_VALIDATE_FATAL {type(exc).__name__}: {exc}")
    print(traceback.format_exc())
finally:
    print(f"DONETSK_DECAL_VALIDATE_EXIT success={ok}")
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception:
        pass
