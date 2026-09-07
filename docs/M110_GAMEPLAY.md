# M110-style Precision DMR Integration

## Purpose

This profile turns the supplied M110-style visual reference into a playable precision/DMR weapon for The Unit while reusing the shared weapon runtime introduced in PR #19.

The generated art package is external game geometry only. It is not manufacturing/CAD data and contains no functional internal firearm mechanism.

## Runtime class

`ATU_M110`

Prototype gameplay intent:

- semi-auto only
- precision-focused ADS spread
- stronger recoil than the carbine profiles
- slower practical shot cadence
- timed reload
- 20-round loaded state including chamber
- shared hit trace / recoil / reload / damage / attachment runtime
- fictionalized balance values only

## Generated external art package

`M110_GameReady_Source.zip`

Contains:

- `SM_M110_Body.glb`
- `SM_M110_Stock.glb`
- `SM_M110_Magazine.glb`
- `SM_M110_Scope.glb`
- `SM_M110_MuzzleDevice.glb`
- `SM_M110_Bipod.glb`
- `M110_Reference_Assembled.glb`
- `M110_socket_manifest.json`

## Attachment mapping

| Art piece | Runtime slot | Socket |
|---|---|---|
| Scope | `Optic` | `optic_socket` |
| Stock | `Stock` | `stock_socket` |
| Magazine | `Magazine` | `magazine_socket` |
| Suppressor-like muzzle device | `Muzzle` | `muzzle_device_socket` |
| Bipod | `Bipod` | `bipod_socket` |

PR #19 adds `Bipod` as a first-class attachment slot so precision weapons do not need to misuse `Foregrip` or `SideAccessory`.

## Unreal import target

Suggested folder:

`/Game/Weapons/M110/`

Suggested assets:

- `SM_M110_Body`
- `SM_M110_Stock`
- `SM_M110_Magazine`
- `SM_M110_Scope`
- `SM_M110_MuzzleDevice`
- `SM_M110_Bipod`
- attachment definitions for each removable part
- one `UTUWeaponLoadoutData` matching the supplied reference configuration

## Production validation

Before shipping or merging the weapon as content-complete:

1. UE 5.7 C++/UHT build succeeds.
2. `TheUnit.Combat.M110.Runtime` passes.
3. Import scale and forward axis are correct.
4. Scope eye relief / ADS alignment is visually comfortable in first person.
5. Magazine fits the magwell and clears reload animation.
6. Muzzle device clears handguard and muzzle VFX socket.
7. Bipod clears the handguard and ground in folded/deployed presentation states.
8. Recoil and cadence are tuned for gameplay, not copied from real-world specifications.
9. First-person and third-person meshes do not clip operator hands/gear.
10. LODs, collision, UVs, PBR materials and animation presentation receive a production art pass.

## Current limitations

- The generated GLBs are high-detail blockout/game-source art, not final production meshes.
- Bipod is currently an attachment slot/visual boundary; deployment mechanics are a later gameplay feature.
- Scope magnification / render-to-texture optics are not implemented yet.
- No UE compile, automation run or PIE validation has been performed from this chat environment.
