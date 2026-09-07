# Special Operations Command Center Hub

## Intent

The front-end for The Unit should feel like a physical secure headquarters, not a conventional game menu. The player enters a walkable special-operations command center, prepares equipment, test-fires weapons, reviews the mission, and deploys from inside that space.

The supplied Operator reference video is the visual/flow anchor for the facility pass. The Unit should borrow the interaction language and institutional mood, not copy logos, proprietary UI, or exact room geometry.

## Reference cues to preserve

The supplied clip establishes several strong cues:

- a narrow secure hallway before the operations space
- painted institutional walls rather than a glamorous sci-fi lobby
- commercial carpet/tile flooring
- suspended/drop ceiling with repeated fluorescent fixtures
- simple secure doors with restrained signage
- weapon wall storage visible as part of the physical room
- locked cage-style equipment storage
- conference/briefing furniture in the same believable facility language

The Unit deliberately changes the mission-interface device: there is no briefing laptop or desktop workstation. Each operator carries the mission interface on a chest-mounted MX50-style tactical tablet attached to the chest rig.

## The Unit layout

The Unit expands the preparation concept into distinct spaces connected by the secure corridor.

### Secure corridor

The player spawn/prep approach begins in a government-facility style corridor.

Graybox requirements:

- narrow corridor footprint
- segmented walls with real door openings
- threshold floors into every room
- low drop ceiling
- repeated fluorescent fixture placeholders
- minimal door signage for ARMORY, CAGE, BRIEFING, RANGE

### Armory

Purpose: choose and configure weapons, then test them without entering a mission.

Current systems connected:

- Primary selection: TU-556, AK-105, M110
- Secondary selection: G34 CM, RGR Five7
- weapon loadout state remains separate per Primary/Secondary actor
- weapon customization benches use the weapon-only armory view

Room language:

- full weapon wall
- utilitarian benches
- parts/tool surfaces
- direct route to the test range

Target production flow:

1. walk to weapon wall or bench
2. press F
3. choose weapon or open its attachment/customization view
4. close station UI
5. walk directly to the range
6. test fire the currently selected weapon
7. return to bench and adjust

### Test-fire range

Purpose: let the player validate weapons before committing the mission loadout.

Prototype includes:

- firing line
- separated lanes
- backstop
- reusable range targets using Unreal's normal damage path
- target health/hit/reset state

Future production layer:

- paper/steel target art
- reset controls
- shot grouping
- timer/drill scoring
- recoil and zeroing feedback
- ammo replenishment boundary for command-center testing

### The Cage

Purpose: choose everything worn or carried that is not the firearm itself.

Physical language:

- locked mesh equipment bays
- lockers
- armor/helmet/uniform displays
- gear customization bench

Station view:

- Melee
- Equipment
- eventually uniform, armor, helmet, carrier, belt, pack, NVG and accessory selections from the modular operator system

The ballistic helmet already being integrated into the operator/equipment branch is intended to become one of these physical Cage selections later.

### Briefing room

Purpose: squad gathering, mission/intel review, planning discussion and final deployment readiness.

Physical language:

- conference table
- chairs
- large wall display / whiteboard area
- no laptop or desktop mission terminal on the table

The mission interface belongs to the operator.

## MX50 chest-mounted tactical tablet

The MX50-style tactical tablet is mounted on the operator's chest rig, not placed in the room.

Prototype runtime contract:

- third-person tablet presentation attaches at `tablet_chest_socket`
- the final plate carrier/chest rig should author that socket
- while stowed, the tablet is represented on the operator's chest
- entering briefing use raises an owner-only first-person tablet presentation
- raising the tablet stops ADS/fire and hides the current firearm
- closing the MX50 lowers it back to the chest presentation and restores the current firearm
- `BP_OnMX50RaisedChanged` is the animation/art bridge for the future hand pull-up / re-stow montage

The current primitive mesh is only a placeholder for gameplay and attachment validation. Final MX50-style art should be a rugged tactical tablet silhouette suitable for a chest mount; it should not depend on proprietary product CAD.

Target briefing interaction flow:

1. squad walks into the briefing room
2. player approaches the briefing interaction area
3. press F
4. operator raises the MX50 from the chest rig
5. MX50 displays active operations
6. select an operation
7. review mission objective, map, imagery, known threats and entry plan
8. review team assignments and current loadout
9. confirm readiness
10. close/lower the MX50 or commit to deployment

The mission interface should feel like tactical software running on equipment the operator physically carries, not like floating game cards and not like a desktop computer.

Longer-term target:

- allow the MX50 to be raised outside the briefing room for appropriate in-mission functions such as map/intel/task review
- keep mission-launch authority and pre-mission readiness rules separate from generic tablet access
- show teammates physically using their own chest-mounted tablets in co-op

No unrestricted in-mission tablet access is claimed in the current pass.

## Current interaction model

`F` is the production command-center interaction key.

The armed operator traces from the first-person camera for `ATU_CommandCenterStation` actors within command-center interaction range.

Station routing:

- Armory -> weapon selection UI
- Weapon Bench -> weapon customization view
- Cage -> gear/equipment selection UI
- Uniform Bench -> gear/equipment selection UI
- Briefing -> raise chest-mounted MX50 and open mission/intel interface
- Mission Launch -> deployment ready check
- Test Range -> returns to gameplay input and equips the selected Primary

The old portable armory hotkey remains disabled by default and exists only as an opt-in development shortcut.

## Graybox generator

`ATU_CommandCenterGenerator` builds the initial walkable headquarters blockout.

It currently generates:

- secure corridor
- physically open room doorways and floor thresholds
- armory weapon wall
- weapon benches
- Cage mesh storage bays and lockers
- gear/uniform bench
- briefing conference furniture
- briefing wall display
- no laptop/table-mounted mission device
- test range lanes/backstop
- interaction station markers
- runtime station actors
- reusable live-fire targets

This generator is a prototype/layout tool. Production art should replace primitive geometry without changing the gameplay station contracts.

## Production art direction

Do not make this look like a glossy esports lobby or futuristic spaceship.

Prefer:

- painted CMU/drywall institutional walls
- slightly worn floor tile / commercial carpet
- acoustic ceiling tiles
- fluorescent fixtures
- steel cage partitions
- pegboard/slatwall/rack weapon storage
- utilitarian tables/chairs
- subdued signage
- practical security cameras/readers
- believable chest-rig mounting hardware for the MX50

The environment can become more premium than the reference, but should remain believable as a working special-operations facility.

## Multiplayer / co-op target

The final command center should support the squad preparing together.

Target rules:

- each operator has an independent loadout
- squad members are visible moving around the same hub
- players can use separate benches simultaneously
- every operator carries an independent chest-mounted MX50
- briefing/readiness state can be reviewed on each player's tablet
- host/leader authority for final mission launch is explicit
- ready state should be visible physically or on the briefing wall display

No multiplayer implementation is claimed in the current graybox pass.

## Validation gate

Before this hub PR leaves draft:

1. UE 5.7 UHT/C++ compile succeeds.
2. `ATU_CommandCenterGenerator` can be placed in a test map without construction errors.
3. all four destination rooms are physically walkable from the corridor.
4. F opens the correct UI at Armory, Weapon Bench, Cage, Uniform Bench and Briefing stations.
5. the portable-armory debug hotkey is disabled by default.
6. weapon/gear UI input never fires a weapon through the menu.
7. leaving a station restores normal game input.
8. selected Primary/Secondary state survives walking between Armory and Range.
9. range targets take damage and reset correctly.
10. there is no laptop/tablet mission device generated in the briefing room.
11. briefing interaction raises the operator's chest-mounted MX50 presentation.
12. the current firearm hides while the MX50 is raised and returns when it is lowered.
13. final modular chest rigs provide/validate `tablet_chest_socket` placement.
14. mission launch remains a deliberate final action, not an accidental proximity trigger.
15. the visual art pass preserves the secure institutional mood established by the supplied reference.

No UE compile, automation execution or PIE validation is claimed yet.
