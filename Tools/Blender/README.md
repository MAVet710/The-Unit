# Blender character-pipeline helpers

## SWAT Operator candidate separation

`swat_operator_prepare.py` is a conservative preprocessing helper for the user-supplied `SWAT+Operator+CGTrader.blend` file.

It does **not** classify or delete geometry. It:

1. finds the inspected character mesh `sol_8_low`;
2. duplicates it in memory;
3. separates disconnected geometry islands on the duplicate only;
4. places the candidates in `TU_SWAT_SeparationCandidates`;
5. records vertex/polygon counts, materials, bounding boxes, armature modifiers and strongest bone influences;
6. writes `swat_loose_parts_report.json`;
7. saves a new `*_TU_working.blend` copy.

The original object and original `.blend` path are preserved. The script also refuses an output path that resolves to the source file.

### Recommended command

From the repository root, with Blender available on `PATH`:

```powershell
blender --background --disable-autoexec "C:\path\to\SWAT+Operator+CGTrader.blend" --python "Tools\Blender\swat_operator_prepare.py"
```

`--disable-autoexec` prevents embedded scripts/drivers in an untrusted source file from automatically executing. The explicitly supplied The Unit script still runs.

### Optional paths

PowerShell example:

```powershell
$env:TU_SWAT_WORKING_BLEND = "C:\path\to\SWAT_Operator_TU_working.blend"
$env:TU_SWAT_REPORT_JSON = "C:\path\to\swat_loose_parts_report.json"
blender --background --disable-autoexec "C:\path\to\SWAT+Operator+CGTrader.blend" --python "Tools\Blender\swat_operator_prepare.py"
```

### What to do with the report

The report is evidence for manual classification, not an automatic gear classifier. Use the candidate mesh visually together with material names, location/bounds and strongest bone influences to identify likely helmet, vest, belt, backpack, body/uniform and other pieces.

A candidate index by itself is never enough to declare a slot. Some gear may be connected to clothing/body geometry and require manual edit-mode separation instead of loose-part separation.

After classification, follow `docs/SWAT_OPERATOR_INTEGRATION.md` to verify body coverage, preserve weights, create Unreal sockets/retarget assets and author `UTUEquipmentDefinition` / `UTUOperatorLoadoutData` assets.
