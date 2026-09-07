# Special Operations Command Center Hub

## Intent

The front-end for The Unit should feel like a physical secure headquarters, not a conventional game menu. The player enters a walkable special-operations command center, prepares equipment, test-fires weapons, reviews the mission, and deploys from inside that space.

The supplied Operator reference video is the visual/flow anchor for this pass. The Unit should borrow the *interaction language* and institutional mood, not copy logos, proprietary UI, or exact room geometry.

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
- a physical laptop/terminal as the bridge from the 3D world into mission planning
- classified-workstation presentation before the actual mission screen
- mission selection progressing from operations list -> briefing/intel -> map/plan -> deploy

## The Unit layout

The Unit expands that concept into distinct preparation spaces connected by the secure corridor.

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

Purpose: mission/intel selection and final deployment readiness.

Physical language from the supplied reference:

- conference table
- chairs
- large wall display / whiteboard area
- physical secure laptop on the table

The laptop is the primary mission UI anchor.

Interaction flow:

1. walk to the briefing laptop
2. press F
3. enter a classified-workstation style screen
4. browse active operations
5. select an operation
6. view mission area/intel/parameters
7. confirm loadout/readiness
8. deploy

The UI should feel like software running on an operations workstation rather than floating game cards.

## Current interaction model

`F` is the production command-center interaction key.

The armed operator traces from the first-person camera for `ATU_CommandCenterStation` actors within command-center interaction range.

Station routing:

- Armory -> weapon selection UI
- Weapon Bench -> weapon customization view
- Cage -> gear/equipment selection UI
- Uniform Bench -> gear/equipment selection UI
- Briefing -> mission briefing workstation
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
- physical laptop geometry
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
- old-but-secure workstation hardware
- subdued signage
- practical security cameras/readers
- realistic clutter kept under control

The environment can become more premium than the reference, but should remain believable as a working special-operations facility.

## Multiplayer / co-op target

The final command center should support the squad preparing together.

Target rules:

- each operator has an independent loadout
- squad members are visible moving around the same hub
- players can use separate benches simultaneously
- briefing terminal exposes mission selection/readiness to the squad
- host/leader authority for final mission launch is explicit
- ready state should be visible physically or on the briefing display

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
10. briefing laptop opens the mission workstation flow.
11. mission launch remains a deliberate final action, not an accidental proximity trigger.
12. the visual art pass preserves the secure institutional mood established by the supplied reference.

No UE compile, automation execution or PIE validation is claimed yet.
