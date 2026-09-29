import unreal

MAT_DEST = "/Game/TheUnit/Donetsk/Materials"

assets = unreal.EditorAssetLibrary.list_assets(
    MAT_DEST, recursive=False, include_folder=False
)
changed = 0
failed = 0

for path in assets:
    material = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(material, unreal.Material):
        continue
    try:
        material.set_editor_property("used_with_nanite", True)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        print("NANITE_USAGE_OK", path)
        changed += 1
    except Exception as exc:
        print("NANITE_USAGE_FAIL", path, exc)
        failed += 1

print(f"DONETSK_NANITE_MATERIALS changed={changed} failed={failed}")
unreal.SystemLibrary.quit_editor()
