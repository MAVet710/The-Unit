# MX50 Tactical Tablet

## Purpose

The MX50 is an operator-worn tactical tablet mounted to the chest rig. It is not a briefing-room computer and is not furniture. The same physical device follows the operator from the Special Operations Command Center into missions.

PR #22 establishes the chest-mounted presentation and command-center interaction. This branch adds the tablet software/state layer.

## Physical behavior

Normal state:

- MX50 is stowed on `tablet_chest_socket` on the operator chest rig.
- the firearm remains usable normally.

Raised state:

- briefing interaction or the field tablet command raises the MX50 into first-person view.
- the active firearm stops firing, leaves ADS and is hidden while the tablet owns the operator's attention.
- the local UI is displayed over the raised tablet presentation.
- stowing the MX50 restores normal game input and the current firearm.

Final animation target:

1. support hand releases the weapon / weapon lowers
2. operator pulls the MX50 from the chest mount
3. tablet moves into readable first-person position
4. hands support and operate the device
5. close action reverses the sequence and re-seats the tablet on the carrier

`BP_OnMX50RaisedChanged(bool)` remains the animation/art bridge for that final presentation.

## Input

Prototype field input:

- `T` toggles the MX50 through `ATU_PlayerController`.
- the command-center Briefing station still uses `F` and opens the same MX50 with the station mission context.
- field toggle is blocked while the Armory UI is open or melee is actively equipped.

`ATU_GameMode` now selects `ATU_PlayerController` by default so native maps inherit the tactical controller unless a Blueprint GameMode explicitly overrides it.

## Persistent tablet state

`UTUMX50TabletComponent` is owned by `ATU_PlayerController`.

It stores:

- mission id
- mission title
- objective summary
- area label
- threat summary
- team summary
- drone/video availability
- deployment authorization
- active MX50 page

The state survives closing/reopening the widget for the life of the local controller. A briefing station updates the same state instead of creating a separate briefing-only data model.

## Pages

The native MX50 UI currently exposes six pages.

### Mission

Displays:

- operation title
- mission id
- area
- objective
- deployment authorization state

### Map

Reserved for the actual tactical map/floor-plan layer.

Target overlays:

- entry points
- objectives
- rally points
- known threats
- squad marks
- route planning

The prototype deliberately does not invent map geometry when mission data has not supplied it.

### Intel

Target content:

- threat assessment
- photographs
- documents
- target imagery
- ROE notes
- collected evidence / newly discovered intelligence

### Team

Target content:

- callsigns
- roles
- readiness
- health/status
- leader authority
- co-op deployment state

### Drone / Video

Integration boundary for:

- The Unit FPV drone
- body cameras
- remote observation cameras
- other authorized tactical feeds

The page currently uses a `bDroneFeedAvailable` state flag and intentionally does not compile against the separate FPV PR. After the FPV branch lands, the feed layer should provide the render target and link telemetry through this boundary.

### Loadout

Currently reads operator-side runtime information such as:

- selected carried weight
- selected melee item
- active primary/secondary slot

Future equipment merge should add:

- armor / helmet condition
- carrier configuration
- belt / pack inventory
- gadgets
- ammunition state
- medical / mission equipment

## Command-center relationship

The Briefing Room is still important, but it is a physical team-planning space rather than the location of the mission computer.

Target flow:

1. squad enters Briefing Room
2. leader/team uses the physical briefing interaction point
3. each participating operator raises their chest-mounted MX50
4. mission context is loaded into the tablet
5. team reviews Mission / Map / Intel / Team / Loadout
6. leader uses explicit deployment authorization / mission-launch flow
7. tablets are stowed and the squad deploys

## In-mission relationship

The MX50 is intended to remain useful after deployment.

Field target:

- re-check objective
- review map and route
- inspect newly received intel
- see squad status
- open available drone/bodycam feeds
- inspect current carried loadout/status

Using the tablet must remain a deliberate tactical tradeoff: while raised, normal firearm handling is suppressed.

## Automation

`TheUnit.MX50.StateAndPages`

Covers:

- default mission/page state
- mission-context update
- page cycling and wraparound
- drone-feed availability
- deployment authorization
- full snapshot replacement

`TheUnit.MX50.ControllerWiring`

Covers:

- native GameMode -> tactical PlayerController wiring
- persistent MX50 component existence on the controller
- default controller tablet context

## Validation gate

Do not treat this feature as runtime-complete until:

1. UE 5.7 UHT/C++ compile succeeds.
2. `TheUnit.MX50.*` automation actually executes and passes.
3. `T` reliably raises/stows the same chest-mounted tablet in PIE.
4. briefing `F` interaction updates persistent mission context.
5. the last selected page persists across stow/re-raise.
6. firearm input cannot leak through while the tablet UI is active.
7. firearm visibility/state restores correctly after stow.
8. the final SWAT carrier validates `tablet_chest_socket` placement.
9. first-person pull-off-chest / re-stow animation is authored and tested.
10. multiplayer ownership/replication of third-person tablet presentation is validated.
11. FPV render-target integration is implemented only after the FPV branch is merged/available.

No UE compile, automation execution, PIE validation, final tablet art, animation, live map, network replication or FPV render feed is claimed by this branch yet.
