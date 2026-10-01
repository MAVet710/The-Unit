import json
import traceback
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
LEDGER_PATH = ROOT / "Tools" / "Reference" / "donetsk_surface_source_ledger.json"
SOURCE_ROOT = ROOT / "ExternalAssets" / "GeneratedDonetsk" / "Textures"
MAT_ROOT = "/Game/TheUnit/Donetsk/Materials"
TEX_ROOT = f"{MAT_ROOT}/Textures"
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

INSTANCE_SPECS = {
    "Asphalt": ("surface", "Asphalt", "asphalt", 0.42, 0.05, 0.0, 0.96),
    "Paving": ("surface", "Paving", "paving", 0.55, 0.03, 0.0, 0.98),
    "AgedConcrete": ("surface", "Concrete", "aged_concrete", 0.70, 0.02, 0.0, 0.91),
    "CurbConcrete": ("surface", "Concrete", "curb_concrete", 0.62, 0.04, 0.0, 1.02),
    "Plaster": ("surface", "WhitePlaster", "plaster", 0.90, 0.03, 0.0, 0.99),
    "PanelWarm": ("surface", "PanelWarm", "panel_warm", 0.82, 0.04, 0.0, 0.98),
    "PanelCool": ("surface", "PanelCool", "panel_cool", 0.82, 0.04, 0.0, 0.98),
    "Brick": ("surface", "Brick", "brick", 0.72, 0.03, 0.0, 0.96),
    "GalvanizedMetal": ("surface", "Galvanized", "galvanized_metal", 0.45, 0.02, 0.92, 1.0),
    "RustedSteel": ("surface", "RustSteel", "rusted_steel", 0.62, 0.03, 0.68, 0.95),
    "Wood": ("surface", "Wood", "wood", 0.62, 0.02, 0.0, 0.97),
    "Soil": ("surface", "DrySoil", "soil", 0.86, 0.02, 0.0, 0.95),
    "WetGround": ("surface", "DrySoil", "wet_ground", 0.74, 0.82, 0.0, 0.82),
    "Glass": ("glass", "DarkGlass", "glass", 0.18, 0.0, 0.0, 1.0),
    "Foliage": ("foliage", "TreeLeaves", "foliage", 0.72, 0.02, 0.0, 1.0),
    "VehiclePaint": ("surface", "VehiclePaint", "vehicle_paint", 0.34, 0.02, 0.72, 1.0),
}

MASTER_NAMES = {
    "surface": "M_Donetsk_Surface_Master",
    "glass": "M_Donetsk_Glass_Master",
    "foliage": "M_Donetsk_Foliage_Master",
}


def trace(message):
    print(f"DONETSK_MATERIAL_BUILD {message}")


def asset(path):
    return unreal.EditorAssetLibrary.load_asset(path)


def import_texture(prefix, kind):
    src = SOURCE_ROOT / f"{prefix}_{kind}.png"
    if not src.exists():
        raise FileNotFoundError(src)
    name = f"T_Donetsk_{prefix}_{kind}"
    dest = f"{TEX_ROOT}/{name}"
    task = unreal.AssetImportTask()
    task.filename = str(src)
    task.destination_path = TEX_ROOT
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    ASSET_TOOLS.import_asset_tasks([task])
    tex = asset(dest)
    if not isinstance(tex, unreal.Texture2D):
        raise RuntimeError(f"Texture import failed: {dest}")
    if kind == "Normal":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property(
            "compression_settings",
            unreal.TextureCompressionSettings.TC_NORMALMAP,
        )
    elif kind in ("Roughness", "Metallic"):
        tex.set_editor_property("srgb", False)
        tex.set_editor_property(
            "compression_settings",
            unreal.TextureCompressionSettings.TC_MASKS,
        )
    else:
        tex.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    return tex


def ensure_all_textures():
    prefixes = sorted({spec[1] for spec in INSTANCE_SPECS.values()})
    result = {}
    for prefix in prefixes:
        for kind in ("BaseColor", "Normal", "Roughness", "Metallic"):
            result[(prefix, kind)] = import_texture(prefix, kind)
            trace(f"texture {prefix}/{kind}")
    return result


def create_material(name):
    path = f"{MAT_ROOT}/{name}"
    mat = asset(path)
    if not isinstance(mat, unreal.Material):
        mat = ASSET_TOOLS.create_asset(
            name, MAT_ROOT, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(mat, unreal.Material):
        raise RuntimeError(f"Could not create material {path}")
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    try:
        mat.set_editor_property("used_with_nanite", True)
        mat.set_editor_property("used_with_instanced_static_meshes", True)
    except Exception as exc:
        trace(f"usage warning {name}: {exc}")
    return mat


def expr(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def scalar(mat, name, value, x, y):
    node = expr(mat, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", float(value))
    return node


def vector(mat, name, value, x, y):
    node = expr(mat, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property(
        "default_value",
        unreal.LinearColor(float(value[0]), float(value[1]), float(value[2]), float(value[3])),
    )
    return node


def texparam(mat, name, tex, x, y):
    node = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", tex)
    return node
def connect(a, out_name, b, in_name):
    ok = unreal.MaterialEditingLibrary.connect_material_expressions(
        a, out_name, b, in_name
    )
    return bool(ok)


def connect_prop(node, output, prop):
    return bool(
        unreal.MaterialEditingLibrary.connect_material_property(node, output, prop)
    )


def build_surface_graph(mat, defaults):
    uv = expr(mat, unreal.MaterialExpressionTextureCoordinate, -1250, -320)
    uv_scale = scalar(mat, "UVScale", 1.0, -1250, -160)
    uv_mul = expr(mat, unreal.MaterialExpressionMultiply, -1030, -260)
    connect(uv, "", uv_mul, "A")
    connect(uv_scale, "", uv_mul, "B")

    base = texparam(mat, "BaseColorTex", defaults["BaseColor"], -820, -520)
    normal = texparam(mat, "NormalTex", defaults["Normal"], -820, 280)
    rough = texparam(mat, "RoughnessTex", defaults["Roughness"], -820, -20)
    metal = texparam(mat, "MetallicTex", defaults["Metallic"], -820, 140)
    for node in (base, normal, rough, metal):
        connect(uv_mul, "", node, "Coordinates")

    macro_tint = vector(mat, "MacroTint", (1.0, 1.0, 1.0, 1.0), -580, -620)
    variation = scalar(mat, "Variation", 1.0, -580, -480)
    tint_mul = expr(mat, unreal.MaterialExpressionMultiply, -340, -500)
    color_mul = expr(mat, unreal.MaterialExpressionMultiply, -120, -500)
    connect(base, "RGB", tint_mul, "A")
    connect(macro_tint, "RGB", tint_mul, "B")
    connect(tint_mul, "", color_mul, "A")
    connect(variation, "", color_mul, "B")
    connect_prop(color_mul, "", unreal.MaterialProperty.MP_BASE_COLOR)

    macro_rough = scalar(mat, "MacroRoughness", 1.0, -580, -120)
    rough_mul = expr(mat, unreal.MaterialExpressionMultiply, -340, -80)
    connect(rough, "R", rough_mul, "A")
    connect(macro_rough, "", rough_mul, "B")
    wetness = scalar(mat, "Wetness", 0.0, -340, 60)
    wet_rough = expr(mat, unreal.MaterialExpressionConstant, -340, 190)
    wet_rough.set_editor_property("r", 0.10)
    rough_lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -90, 30)
    connect(rough_mul, "", rough_lerp, "A")
    connect(wet_rough, "", rough_lerp, "B")
    connect(wetness, "", rough_lerp, "Alpha")
    connect_prop(rough_lerp, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metallic_scale = scalar(mat, "MetallicScale", 1.0, -580, 150)
    metal_mul = expr(mat, unreal.MaterialExpressionMultiply, -320, 160)
    connect(metal, "R", metal_mul, "A")
    connect(metallic_scale, "", metal_mul, "B")
    connect_prop(metal_mul, "", unreal.MaterialProperty.MP_METALLIC)

    detail_strength = scalar(mat, "DetailNormalStrength", 1.0, -560, 330)
    flat = expr(mat, unreal.MaterialExpressionConstant3Vector, -560, 470)
    flat.set_editor_property("constant", unreal.LinearColor(0.5, 0.5, 1.0, 1.0))
    normal_lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -260, 350)
    connect(flat, "", normal_lerp, "A")
    connect(normal, "RGB", normal_lerp, "B")
    connect(detail_strength, "", normal_lerp, "Alpha")
    connect_prop(normal_lerp, "", unreal.MaterialProperty.MP_NORMAL)

    # Exposed compatibility control. Current hero meshes have valid UVs, but the
    # parameter remains part of the contract for future world-aligned fallback.
    scalar(mat, "WorldAlignedFallback", 0.0, -560, 620)
    return base


def build_master(kind, textures):
    name = MASTER_NAMES[kind]
    mat = create_material(name)
    default_prefix = {
        "surface": "Concrete",
        "glass": "DarkGlass",
        "foliage": "TreeLeaves",
    }[kind]
    defaults = {
        k: textures[(default_prefix, k)]
        for k in ("BaseColor", "Normal", "Roughness", "Metallic")
    }
    base = build_surface_graph(mat, defaults)

    if kind == "glass":
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        opacity = scalar(mat, "Opacity", 0.35, 40, 220)
        connect_prop(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    elif kind == "foliage":
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property("two_sided", True)
        connect_prop(base, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
        try:
            mat.set_editor_property(
                "shading_model",
                unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE,
            )
        except Exception as exc:
            trace(f"foliage shading model warning: {exc}")
    else:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    trace(f"master {name}")
    return mat


def create_instance(name, parent, textures, prefix, roughness, wetness, metallic, variation):
    asset_name = f"MI_Donetsk_{name}"
    path = f"{MAT_ROOT}/{asset_name}"
    mi = asset(path)
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        mi = ASSET_TOOLS.create_asset(
            asset_name,
            MAT_ROOT,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        raise RuntimeError(f"Could not create instance {path}")
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, parent)
    for kind, param in (
        ("BaseColor", "BaseColorTex"),
        ("Normal", "NormalTex"),
        ("Roughness", "RoughnessTex"),
        ("Metallic", "MetallicTex"),
    ):
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            mi, param, textures[(prefix, kind)]
        )
    values = {
        "UVScale": 1.0,
        "MacroRoughness": roughness,
        "DetailNormalStrength": 1.0,
        "Wetness": wetness,
        "Variation": variation,
        "WorldAlignedFallback": 0.0,
        "MetallicScale": metallic,
    }
    for param, value in values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            mi, param, float(value)
        )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        mi, "MacroTint", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    if name == "Glass":
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            mi, "Opacity", 0.32
        )
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    trace(f"instance {asset_name}")
    return mi
def main():
    if not LEDGER_PATH.exists():
        raise FileNotFoundError(LEDGER_PATH)
    ledger = json.loads(LEDGER_PATH.read_text(encoding="utf-8"))
    trace(f"ledger surfaces={len(ledger.get('surfaces', []))}")

    textures = ensure_all_textures()
    masters = {
        kind: build_master(kind, textures)
        for kind in ("surface", "glass", "foliage")
    }
    instances = {}
    for name, spec in INSTANCE_SPECS.items():
        kind, prefix, physical, roughness, wetness, metallic, variation = spec
        instances[name] = create_instance(
            name,
            masters[kind],
            textures,
            prefix,
            roughness,
            wetness,
            metallic,
            variation,
        )

    unreal.EditorAssetLibrary.save_directory(
        MAT_ROOT, only_if_is_dirty=False, recursive=True
    )
    trace(f"complete masters={len(masters)} instances={len(instances)}")
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
