# SWAT Operator Integration and Modular Equipment

## Purpose

Turn the user-supplied SWAT Operator source into the first playable third-person operator for The Unit without locking the game to one combined character mesh. The runtime code is data-driven so the supplied model can serve as the base appearance while helmets, armor, rigs, packs, belts and accessories become independently replaceable as the art is separated or replaced.

## What the supplied source currently contains

The September 7, 2026 read-only inspection recorded:

- `SWAT+Operator.fbx` — 3,700,748 bytes.
- `SWAT+Operator+CGTrader.blend` — 16,105,872 bytes.
- `Textures.zip` — 216,936,820 bytes.
- 32 mesh objects in the FBX/Blender source.
- The character itself is the single combined `sol_8_low` mesh.
- The other 31 mesh objects are weapon parts, not independently separated character gear.
- The character armature uses `mixamorig:` bone naming.
- The Blender source does not already preserve helmet/vest/etc. as separate character objects.

The original files remain outside Git. Do not commit marketplace/source binaries unless their exact redistribution terms permit it.

## Runtime architecture added by `feature/operator-equipment-core`

### `ATU_ModularOperatorCharacter`

A subclass of `ATU_OperatorCharacter`. It owns the operator appearance/loadout layer without replacing the existing movement/combat character. This intentionally reduces conflicts with other work such as FPV deployment, health and death lifecycle changes.

### `UTUOperatorAppearanceData`

Defines:

- third-person body mesh
- third-person animation class
- body transform
- owner visibility policy
- optional first-person arms mesh
- first-person animation class
- first-person transform
- default equipment loadout

The first imported SWAT body should be assigned here rather than hard-coded into C++.

### `UTUEquipmentDefinition`

Defines one removable item:

- stable item id
- tactical slot
- weight
- weighted skeletal or rigid static third-person visual
- socket and relative transform
- leader-pose option
- optional dedicated first-person visual

Weighted skeletal gear is intended for items such as clothing/vest geometry that shares the operator skeleton. Rigid static gear is intended for items such as helmets, radios, pouches and accessories that can follow a socket.

### `UTUOperatorLoadoutData`

A named list of equipment definitions. One item per slot is preferred. If duplicate slots are authored, the later item intentionally replaces the earlier item and emits a warning.

### `UTUOperatorEquipmentComponent`

Owns equipped-slot state and dynamically creates/removes the visual components. It supports:

- equip/replace
- unequip
- clear loadout
- apply loadout
- rebuild visuals after a body/arms change
- total equipment weight
- separate first-person and third-person visibility
- same-skeleton leader-pose validation with a socket-attachment fallback

## Recommended Unreal content layout

Keep source/imported content separate from gameplay definitions:

```text
/Game/Characters/Operators/SWAT/
  Meshes/
    SK_SWAT_Combined_Source
    SK_SWAT_BaseBody
    SK_SWAT_Vest
    SK_SWAT_Clothing_...
  Gear/
    SM_SWAT_Helmet
    SM_SWAT_Radio
    SM_SWAT_Pouch_...
  Materials/
  Textures/
  Animation/
    IK_SWAT_Mixamo
    RTG_SWAT_To_TheUnit
  Data/
    DA_Operator_SWAT
    DA_Loadout_SWAT_Default
    EQ_SWAT_Helmet
    EQ_SWAT_Vest
    EQ_SWAT_Backpack
    ...
```

Names are conventions, not hard runtime requirements.

## Local art conversion sequence

### 1. Preserve an untouched source copy

Keep the original FBX/Blend/texture archive read-only in `ExternalAssets/`. Work from copies. Do not destructively edit the only original.

### 2. Import the combined body as a reference first

Import `sol_8_low` with its Mixamo armature as a skeletal mesh and preserve its existing skinning/material assignments. This gives the project a visual reference operator before modular separation is finished.

Suggested temporary asset:

`/Game/Characters/Operators/SWAT/Meshes/SK_SWAT_Combined_Source`

Assign it to `UTUOperatorAppearanceData.ThirdPersonBodyMesh` and verify scale/orientation before doing separation work.

### 3. Build the animation bridge

The source uses `mixamorig:` bones. Create an IK Rig for the imported skeleton and an IK Retargeter to the animation skeleton selected for The Unit. Do not rename bones destructively just to imitate the Unreal mannequin; preserve a clean retarget boundary.

Acceptance:

- idle/walk/run/crouch do not collapse shoulders/hips
- hands remain usable for weapon alignment
- foot placement and root direction are correct
- no 90-degree import rotation is being hidden by animation assets

### 4. Separate character gear in Blender

Duplicate the working `.blend`, then identify geometry islands/material regions belonging to:

- underlying body/uniform
- helmet/headwear
- headset
- eyewear/facewear
- NVG hardware
- torso armor / plate carrier
- chest rig
- backpack
- battle belt
- hip accessories
- gloves
- knee pads
- footwear

Do not classify the 31 weapon-part objects as operator gear simply because they are separate meshes.

For weighted gear, separate the geometry while preserving vertex groups/skin weights and the same armature. For rigid gear, it is acceptable to export a static mesh and attach it to a validated body socket.

### 5. Verify the body underneath removable gear

The current combined asset may omit geometry that was never expected to be seen beneath the vest, helmet or other pieces. Before treating an item as removable:

- hide the candidate gear
- inspect for missing torso/head/limb surfaces
- inspect UV/material continuity
- repair missing surfaces or substitute a complete base body/uniform

A successful mesh split is not enough if unequipping an item exposes holes.

### 6. Export modular pieces

Weighted skeletal pieces must use the exact same skeleton as the chosen SWAT base body if `bUseLeaderPose` is enabled. The runtime equipment component checks skeleton identity; incompatible pieces remain socket-attached and emit a warning instead of silently pretending to be compatible.

Rigid pieces should be exported with sane pivots. Create/validate sockets on the operator skeleton for items such as:

- `head_gear`
- `headset`
- `nvg_mount`
- `spine_backpack`
- `belt_left`
- `belt_right`
- `chest_accessory`

The exact socket names can differ; they are authored per `UTUEquipmentDefinition`.

### 7. Author equipment data assets

Create one `UTUEquipmentDefinition` asset per removable item. Set:

- `ItemId`
- `DisplayName`
- `Slot`
- `WeightKg`
- skeletal OR static visual
- socket/transform if rigid
- first-person visual only if the player should actually see that item

Avoid assigning both a skeletal and static mesh for the same perspective. The skeletal visual has precedence in the prototype component.

### 8. Create the SWAT appearance and default loadout

Create `DA_Operator_SWAT` from `UTUOperatorAppearanceData` and `DA_Loadout_SWAT_Default` from `UTUOperatorLoadoutData`.

The appearance asset should point to the clean base body, not permanently merged gear once modular assets exist. The default loadout then reconstructs the intended SWAT look from equipment definitions.

## First-person rules

The existing base character owns a separate first-person arms mesh. Third-person body/gear is hidden from the owning player by default to prevent near-camera clipping.

Only create first-person equipment visuals when they materially need to appear in view, for example gloves/wrist items. A third-person helmet or backpack normally needs no first-person duplicate.

## Equipment acceptance tests

After local assets are imported:

1. Spawn `ATU_ModularOperatorCharacter` through `ATU_GameMode`.
2. Confirm the SWAT base body appears at correct scale and orientation.
3. Validate animation retargeting in idle, walk, sprint, crouch and weapon poses.
4. Equip each slot independently.
5. Replace an occupied slot and confirm the old visual is destroyed.
6. Unequip every removable item and inspect the underlying body for holes.
7. Confirm weighted gear follows animation without double transforms.
8. Confirm rigid gear stays attached during extreme head/spine/hip poses.
9. Check first-person camera for clipping.
10. Check shadows and third-person visibility for remote players.
11. Test crouch/lean/ADS poses for vest/backpack/belt clipping.
12. Confirm total equipment weight matches authored definitions.
13. Run `TheUnit.Operator.Equipment.State` in Unreal Automation.

## Not completed by GitHub-side implementation

Binary asset operations still require the local content tools/editor:

- editing/separating the supplied Blender mesh
- FBX re-export
- Unreal `.uasset` import
- skeleton/socket creation
- IK Rig/Retargeter asset creation
- material/texture setup
- visual clipping fixes

The C++ architecture and validation path are in Git; these art/editor steps cannot be truthfully represented as completed until the supplied source files are processed locally.

## Licensing gate

The third-party asset registry still marks the SWAT listing as `Needs Review`. Before shipping or redistributing derived/source assets, verify the exact assembled-listing terms plus the underlying component attribution/license requirements. Keep source archives outside the public repository unless redistribution permission is explicit.
