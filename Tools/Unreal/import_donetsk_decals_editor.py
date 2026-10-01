import json
import traceback
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = ROOT / "ExternalAssets" / "DonetskDecals"
LEDGER_PATH = ROOT / "Tools" / "Reference" / "donetsk_decal_source_ledger.json"
DEST = "/Game/TheUnit/Donetsk/Decals"
TEX_DEST = f"{DEST}/Textures"
MASTER_NAME = "M_Donetsk_Decal_Master"
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def trace(message):
    print(f"DONETSK_DECAL_IMPORT {message}")


def asset(path):
    return unreal.EditorAssetLibrary.load_asset(path)


def import_texture(name, kind):
    src = SOURCE_ROOT / f"{name}_{kind}.png"
    if not src.exists():
        raise FileNotFoundError(src)
    dest_name = f"T_Donetsk_Decal_{name}_{kind}"
    task = unreal.AssetImportTask()
    task.filename = str(src)
    task.destination_path = TEX_DEST
    task.destination_name = dest_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    ASSET_TOOLS.import_asset_tasks([task])
    tex = asset(f"{TEX_DEST}/{dest_name}")
    if not isinstance(tex, unreal.Texture2D):
        raise RuntimeError(f"Texture import failed: {dest_name}")
    if kind == "Normal":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP
        )
    elif kind in ("Roughness", "Opacity"):
        tex.set_editor_property("srgb", False)
        tex.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_MASKS
        )
    else:
        tex.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    return tex


def ensure_master():
    path = f"{DEST}/{MASTER_NAME}"
    mat = asset(path)
    if not isinstance(mat, unreal.Material):
        mat = ASSET_TOOLS.create_asset(
            MASTER_NAME, DEST, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(mat, unreal.Material):
        raise RuntimeError("Could not create decal master")
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    return mat


def expr(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def texparam(mat, name, tex, x, y):
    node = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", tex)
    return node


def scalar(mat, name, value, x, y):
    node = expr(mat, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", float(value))
    return node


def vector(mat, name, value, x, y):
    node = expr(mat, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property(
        "default_value", unreal.LinearColor(*[float(v) for v in value])
    )
    return node


def connect(a, out_name, b, in_name):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, out_name, b, in_name)


def connect_prop(node, output, prop):
    unreal.MaterialEditingLibrary.connect_material_property(node, output, prop)
def build_master(defaults):
    mat = ensure_master()
    uv = expr(mat, unreal.MaterialExpressionTextureCoordinate, -1200, -250)

    base = texparam(mat, "BaseColorTex", defaults["BaseColor"], -850, -520)
    normal = texparam(mat, "NormalTex", defaults["Normal"], -850, 120)
    rough = texparam(mat, "RoughnessTex", defaults["Roughness"], -850, -90)
    opacity = texparam(mat, "OpacityTex", defaults["Opacity"], -850, 350)
    for node in (base, normal, rough, opacity):
        connect(uv, "", node, "Coordinates")

    tint = vector(mat, "Tint", (1.0, 1.0, 1.0, 1.0), -560, -600)
    base_mul = expr(mat, unreal.MaterialExpressionMultiply, -300, -480)
    connect(base, "RGB", base_mul, "A")
    connect(tint, "RGB", base_mul, "B")
    connect_prop(base_mul, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_scale = scalar(mat, "RoughnessScale", 1.0, -560, -100)
    rough_mul = expr(mat, unreal.MaterialExpressionMultiply, -300, -80)
    connect(rough, "R", rough_mul, "A")
    connect(rough_scale, "", rough_mul, "B")
    connect_prop(rough_mul, "", unreal.MaterialProperty.MP_ROUGHNESS)

    normal_strength = scalar(mat, "NormalStrength", 1.0, -560, 120)
    flat = expr(mat, unreal.MaterialExpressionConstant3Vector, -560, 240)
    flat.set_editor_property(
        "constant", unreal.LinearColor(0.5, 0.5, 1.0, 1.0)
    )
    normal_lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -270, 150)
    connect(flat, "", normal_lerp, "A")
    connect(normal, "RGB", normal_lerp, "B")
    connect(normal_strength, "", normal_lerp, "Alpha")
    connect_prop(normal_lerp, "", unreal.MaterialProperty.MP_NORMAL)

    opacity_scale = scalar(mat, "OpacityScale", 1.0, -560, 390)
    opacity_mul = expr(mat, unreal.MaterialExpressionMultiply, -300, 390)
    connect(opacity, "R", opacity_mul, "A")
    connect(opacity_scale, "", opacity_mul, "B")
    connect_prop(opacity_mul, "", unreal.MaterialProperty.MP_OPACITY)

    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    trace(f"master {path if (path := mat.get_path_name()) else MASTER_NAME}")
    return mat


def create_instance(entry, master, textures):
    name = entry["name"]
    instance_name = entry["instance_name"]
    path = f"{DEST}/{instance_name}"
    mi = asset(path)
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        mi = ASSET_TOOLS.create_asset(
            instance_name,
            DEST,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        raise RuntimeError(f"Could not create {path}")
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, master)
    for kind, param in (
        ("BaseColor", "BaseColorTex"),
        ("Normal", "NormalTex"),
        ("Roughness", "RoughnessTex"),
        ("Opacity", "OpacityTex"),
    ):
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            mi, param, textures[(name, kind)]
        )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        mi, "RoughnessScale", 1.0
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        mi, "NormalStrength", 0.65
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        mi, "OpacityScale", 1.0
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        mi, "Tint", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    trace(f"instance {instance_name}")
    return mi
def main():
    ledger = json.loads(LEDGER_PATH.read_text(encoding="utf-8"))
    entries = ledger.get("decals", [])
    if len(entries) < 12:
        raise RuntimeError(f"Need at least 12 decals, got {len(entries)}")

    textures = {}
    for entry in entries:
        name = entry["name"]
        for kind in ("BaseColor", "Normal", "Roughness", "Opacity"):
            textures[(name, kind)] = import_texture(name, kind)
            trace(f"texture {name}/{kind}")

    first = entries[0]["name"]
    defaults = {
        kind: textures[(first, kind)]
        for kind in ("BaseColor", "Normal", "Roughness", "Opacity")
    }
    master = build_master(defaults)

    instances = []
    for entry in entries:
        instances.append(create_instance(entry, master, textures))

    unreal.EditorAssetLibrary.save_directory(
        DEST, only_if_is_dirty=False, recursive=True
    )
    trace(f"complete instances={len(instances)}")
    return True


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
