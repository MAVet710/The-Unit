import unreal, os, glob, traceback
src=r"C:\Users\ndasi\OneDrive\Documents\GitHub\The-Unit\ExternalAssets\GeneratedDonetsk"
dest="/Game/TheUnit/Donetsk/Production"
tools=unreal.AssetToolsHelpers.get_asset_tools()
count=0
for fn in sorted(glob.glob(os.path.join(src,"*.obj"))):
    name=os.path.splitext(os.path.basename(fn))[0]
    task=unreal.AssetImportTask()
    task.filename=fn
    task.destination_path=dest
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=True
    try:
        tools.import_asset_tasks([task])
        print("IMPORT",name,task.imported_object_paths)
        for path in task.imported_object_paths:
            asset=unreal.EditorAssetLibrary.load_asset(path)
            if isinstance(asset,unreal.StaticMesh):
                try:
                    ns=asset.get_editor_property("nanite_settings"); ns.enabled=True
                    asset.set_editor_property("nanite_settings",ns)
                except Exception as e: print("NANITE_WARN",path,e)
                try: asset.set_editor_property("light_map_resolution",256)
                except Exception: pass
                unreal.EditorAssetLibrary.save_loaded_asset(asset)
        count+=1
    except Exception:
        traceback.print_exc()
print("IMPORTED_NEW",count)
unreal.SystemLibrary.quit_editor()
