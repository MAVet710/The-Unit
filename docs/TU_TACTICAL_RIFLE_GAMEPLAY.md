# TU-556 Tactical Rifle — Gameplay + Asset Integration

## Goal

Turn the user-supplied reference image into a playable, modular first-person rifle for The Unit rather than a static prop. The implementation deliberately separates game behavior from art so the current blockout can be replaced by a production mesh without rewriting firing logic.

## Runtime classes

### `ATU_WeaponBase`

Now supports an opt-in real-time weapon runtime while preserving the original deterministic weapon test behavior:

- camera/viewpoint hitscan trace
- hip-fire vs ADS spread
- attachment spread/recoil modifiers
- controller recoil impulse
- point-damage dispatch on trace impact
- RPM-derived shot cadence
- timed full-auto and burst scheduling
- timed reload lifecycle
- shot result delegate for muzzle flash/audio/impact VFX
- first-person static weapon body
- data-driven attachment component

The original `ATU_WeaponBase` defaults keep timed cadence disabled so existing Phase 1 automation semantics remain stable. Runtime rifle subclasses opt in.

### `ATU_TacticalRifle`

Default playable rifle configuration for the supplied visual reference.

Gameplay identity is intentionally fictionalized as `TU-556 Modular Carbine`; art can resemble the reference without encoding real manufacturing/internal firearm geometry.

Current prototype tuning:

- 30-round magazine plus chamber behavior inherited from weapon mechanics
- semi-auto + full-auto
- 700 RPM gameplay cadence
- timed 2.35 s reload
- 1200 m-class trace ceiling for prototype hit testing
- separate hip/ADS spread
- generic TU-556 ammunition data

These are gameplay values and should be tuned from playtesting, not treated as real firearm specifications.

### `ATU_ArmedOperatorCharacter`

Intermediate operator layer that:

- spawns the tactical rifle
- attaches it to the first-person arms `weapon_socket`
- binds fire / reload / fire-mode controls
- keeps weapon aiming state synchronized with the base operator ADS action

It intentionally subclasses the existing base operator rather than modifying it heavily. Once the modular SWAT operator PR lands, the clean target inheritance is:

```text
ATU_OperatorCharacter
  -> ATU_ArmedOperatorCharacter
      -> ATU_ModularOperatorCharacter
```

This reduces collision with FPV and operator-equipment work.

## Controls

- Left Mouse — fire / hold for full-auto
- Right Mouse — ADS (existing operator mapping)
- R — reload
- B — cycle available fire modes

## Attachment system

`UTUWeaponAttachmentDefinition` supports these slots:

- Optic
- Magnifier
- Laser / illuminator
- Weapon light
- Foregrip
- Muzzle device
- Stock
- Magazine
- Side accessory

Each definition can provide:

- mesh
- socket
- relative fit transform
- recoil multiplier
- spread multiplier
- weight

`UTUWeaponAttachmentComponent` enforces one item per slot, supports replacement/unequip/loadouts, creates runtime visuals, and aggregates handling modifiers.

## Reference-image default configuration

The screenshot can be reconstructed from independent assets/data definitions roughly as:

```text
Rifle body
  Receiver + pistol grip + handguard + barrel foundation

Optic
  Holographic-style sight

Magnifier
  Rear flip-to-side magnifier visual

Laser
  Top/front laser/illuminator module

Foregrip
  Vertical grip

Muzzle
  Long detachable muzzle-device visual

Stock
  Adjustable stock visual

Magazine
  Detachable magazine visual
```

Do not weld these accessories permanently into the production body mesh if we want the armory/loadout system to remain meaningful.

## Existing generated 3D blockout

The first generated GLB contains 43 named visual pieces including:

- `Upper_Receiver`
- `Lower_Receiver`
- `Pistol_Grip`
- `Magazine`
- `Handguard_Main`
- `Top_Rail`
- repeated handguard slot details
- `Vertical_Grip`
- `Solid_Barrel_Shroud`
- `Solid_Muzzle_Device`
- `Optic_Body`
- `Rear_Magnifier`
- `Laser_Module`
- `Stock_Body`

That blockout should be treated as source/reference geometry, not a final production weapon.

## Production asset split

Recommended Unreal content layout:

```text
/Game/Weapons/Rifles/TU556/
  Meshes/
    SM_TU556_Body
  Attachments/
    Optics/SM_TU556_RefOptic
    Magnifiers/SM_TU556_RefMagnifier
    Lasers/SM_TU556_RefLaser
    Foregrips/SM_TU556_RefVerticalGrip
    Muzzle/SM_TU556_RefMuzzle
    Stocks/SM_TU556_RefStock
    Magazines/SM_TU556_RefMagazine
  Data/
    DA_TU556_ReferenceLoadout
    WA_TU556_RefOptic
    WA_TU556_RefMagnifier
    WA_TU556_RefLaser
    WA_TU556_RefVerticalGrip
    WA_TU556_RefMuzzle
    WA_TU556_RefStock
    WA_TU556_RefMagazine
```

Recommended body sockets:

```text
Muzzle
optic_socket
magnifier_socket
laser_socket
light_socket
foregrip_socket
muzzle_device_socket
stock_socket
magazine_socket
side_accessory_socket
```

The exact names are authored in each attachment definition; these are conventions.

## Next art pass

The next model iteration should improve visual fidelity without introducing real-world manufacturing internals:

1. rebuild the receiver silhouette with proper bevels and cleaner proportions
2. refine the handguard cross-section and vents
3. refine the stock and buffer silhouette
4. make magazine, stock, optic, magnifier, laser, foregrip and muzzle visual independent meshes
5. add UVs and tan/black/brown PBR material set
6. establish clean pivots at attachment interfaces
7. add sockets to `SM_TU556_Body`
8. import into Unreal and author the reference attachment data assets
9. align the rifle to the first-person hand/weapon socket
10. add animation/VFX hooks for recoil, muzzle flash, magazine reload and impacts

## Animation boundary

The runtime already exposes enough state/events to drive presentation, but final animations still require local UE/Blender content work:

- first-person idle/ready
- ADS transition
- recoil pose/additive
- reload magazine movement
- empty/reload variants if desired
- fire-mode presentation
- attachment-specific sight alignment

External visible animation is appropriate for the game asset; this project does not require or store real-world internal firearm construction geometry.

## Validation

Run:

- `TheUnit.Combat.WeaponOwnership` — legacy weapon API compatibility
- `TheUnit.Combat.TacticalRifle.Runtime` — timed rifle, attachments and armed-operator wiring

Then perform local UE 5.7 validation for:

- compile/UHT
- first shot trace
- semi-auto cadence
- full-auto cadence while held
- stop-fire behavior
- reload delay
- ADS spread difference
- recoil application
- hit result delegate
- attachment replacement/modifiers
- socket fitting and camera clipping
