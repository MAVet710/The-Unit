import unreal
import os
import glob
import traceback

ROOT = r"C:\Users\ndasi\OneDrive\Documents\GitHub\The-Unit"
SRC = os.path.join(ROOT, "ExternalAssets", "GeneratedDonetsk")
TEX_SRC = os.path.join(SRC, "Textures")
ENV_DEST = "/Game/TheUnit/Donetsk/Environment"
TEX_DEST = "/Game/TheUnit/Donetsk/Materials/Textures"
MAT_DEST = "/Game/TheUnit/Donetsk/Materials"
PROD_DEST = "/Game/TheUnit/Donetsk/Production"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
SURFACES = [
    "Asphalt","Paving","UrbanGrass","DrySoil","RustSteel","Concrete","CoalStone",
    "DarkGlass","PanelWarm","PanelCool","PinkStucco","MetalDark","StalinkaSand",
    "TreeBark","TreeLeaves","WhitePlaster","RoadPaint","Rubber","VehiclePaint",
    "Galvanized","Wood","DirtyWater"
]

def import_file(filename, destination_path, destination_name):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = destination_path
    task.destination_name = destination_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    asset_tools.import_asset_tasks([task])
    return list(task.imported_object_paths)

def set_texture_settings(tex, kind):
    if not tex:
        return
    try:
        if kind in ("Roughness", "Metallic"):
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        elif kind == "Normal":
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
    except Exception as exc:
        print("TEXTURE_SETTINGS_WARN", tex.get_path_name(), exc)

def load_tex(surface, kind):
    return unreal.EditorAssetLibrary.load_asset(
        f"{TEX_DEST}/T_{surface}_{kind}.T_{surface}_{kind}"
    )

def create_or_replace_material(surface):
    asset_path = f"{MAT_DEST}/M_{surface}"
    mat = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not mat:
        mat = asset_tools.create_asset(
            f"M_{surface}", MAT_DEST, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not mat:
        print("MATERIAL_CREATE_FAIL", surface)
        return None

    try:
        # Every Donetsk static-mesh material must compile the Nanite permutation.
        # Without this UE substitutes the default material in packaged builds.
        mat.set_editor_property("used_with_nanite", True)
        mat.set_editor_property("used_with_instanced_static_meshes", True)
    except Exception as exc:
        print("MATERIAL_USAGE_WARN", surface, exc)

    try:
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    except Exception:
        pass

    def sample(kind, x, y):
        tex = load_tex(surface, kind)
        if not tex:
            print("MISSING_TEXTURE", surface, kind)
            return None
        expr = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionTextureSample, x, y
        )
        expr.set_editor_property("texture", tex)
        return expr

    base = sample("BaseColor", -800, -250)
    rough = sample("Roughness", -800, -50)
    metal = sample("Metallic", -800, 150)
    normal = sample("Normal", -800, 350)

    if base:
        unreal.MaterialEditingLibrary.connect_material_property(
            base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
        )
    if rough:
        unreal.MaterialEditingLibrary.connect_material_property(
            rough, "R", unreal.MaterialProperty.MP_ROUGHNESS
        )
    if metal:
        unreal.MaterialEditingLibrary.connect_material_property(
            metal, "R", unreal.MaterialProperty.MP_METALLIC
        )
    if normal:
        unreal.MaterialEditingLibrary.connect_material_property(
            normal, "RGB", unreal.MaterialProperty.MP_NORMAL
        )

    if surface in ("DarkGlass", "DirtyWater"):
        const = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant, -480, 500
        )
        const.set_editor_property("r", 0.35 if surface == "DarkGlass" else 0.15)
        unreal.MaterialEditingLibrary.connect_material_property(
            const, "", unreal.MaterialProperty.MP_SPECULAR
        )

    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat

for surface in SURFACES:
    for kind in ("BaseColor", "Roughness", "Metallic", "Normal"):
        filename = os.path.join(TEX_SRC, f"{surface}_{kind}.png")
        if not os.path.exists(filename):
            print("TEXTURE_FILE_MISSING", filename)
            continue
        imported = import_file(filename, TEX_DEST, f"T_{surface}_{kind}")
        for path in imported:
            tex = unreal.EditorAssetLibrary.load_asset(path)
            if isinstance(tex, unreal.Texture2D):
                set_texture_settings(tex, kind)
print("TEXTURES_IMPORTED")

materials = {}
for surface in SURFACES:
    try:
        materials[surface] = create_or_replace_material(surface)
        print("MATERIAL", surface, bool(materials[surface]))
    except Exception:
        traceback.print_exc()

for fn in sorted(glob.glob(os.path.join(SRC, "SM_Donetsk_Prop_*.obj"))):
    name = os.path.splitext(os.path.basename(fn))[0]
    try:
        imported = import_file(fn, ENV_DEST, name)
        for path in imported:
            asset = unreal.EditorAssetLibrary.load_asset(path)
            if isinstance(asset, unreal.StaticMesh):
                try:
                    settings = asset.get_editor_property("nanite_settings")
                    settings.enabled = True
                    asset.set_editor_property("nanite_settings", settings)
                except Exception as exc:
                    print("NANITE_WARN", path, exc)
                try:
                    asset.set_editor_property("light_map_resolution", 128)
                except Exception:
                    pass
                unreal.EditorAssetLibrary.save_loaded_asset(asset)
        print("PROP_IMPORT", name, imported)
    except Exception:
        traceback.print_exc()

def upgrade_meshes(folder):
    assets = unreal.EditorAssetLibrary.list_assets(folder, recursive=False, include_folder=False)
    for path in assets:
        mesh = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(mesh, unreal.StaticMesh):
            continue
        changed = False
        static_materials = list(mesh.get_editor_property("static_materials"))
        for index, slot in enumerate(static_materials):
            slot_name = str(slot.get_editor_property("imported_material_slot_name"))
            if slot_name == "None" or not slot_name:
                slot_name = str(slot.get_editor_property("material_slot_name"))
            current = mesh.get_material(index)
            if current and current.get_name() in materials:
                slot_name = current.get_name()
            mat = materials.get(slot_name)
            if mat:
                mesh.set_material(index, mat)
                changed = True
        if changed:
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            print("UPGRADED", path)

upgrade_meshes(PROD_DEST)
upgrade_meshes(ENV_DEST)

unreal.EditorAssetLibrary.save_directory(MAT_DEST, only_if_is_dirty=False, recursive=True)
unreal.EditorAssetLibrary.save_directory(ENV_DEST, only_if_is_dirty=False, recursive=True)
print("DONETSK_PBR_ENVIRONMENT_IMPORT_COMPLETE")
