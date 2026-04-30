# Phase 1 Graybox Map Plan — Kill House

## Map Identity

- **Map Name:** `M_TheUnit_Phase1_Killhouse`
- **Phase:** 1D.1
- **Purpose:** A compact tactical test map for movement, room clearing, vertical fighting, weapon testing, AI patrols, objective interaction, and extraction flow.

## Design Intent

This graybox level is intended to be a fast-iteration tactical sandbox with a clear mission loop:

1. Spawn in exterior staging.
2. Select approach (front, side, or rear entry).
3. Clear first floor and push vertically.
4. Secure objective from second-floor comms room.
5. Exfiltrate via rear extraction zone.

The map should stay compact to keep encounter resets and test cycles short while still supporting meaningful vertical and room-to-room decision making.

## High-Level Layout

### Exterior

- **Front staging area** (player spawn and prep space).
- **Front breach entrance** (primary assault route).
- **Side entrance** (flank option).
- **Rear service entrance** (stealth/alternate route).
- **Rear extraction zone** (mission completion endpoint).

### First Floor

- **Lobby** (entry funnel and initial contact zone).
- **Hallway** (choke/crossfire lane).
- **Storage room** (close-quarters clearing test).
- **Office** (cover and corner peeks).
- **Stairwell** (vertical transition and stair combat test).
- **Objective room** (first-floor anchor area for defense/pressure).

### Second Floor

- **Catwalk/Landing** (movement exposure and angle control).
- **Bedroom/Barracks room** (tight CQB node).
- **Comms room** (objective item placement).
- **Overlook angle into first floor** (vertical pressure and overwatch).
- **Alternate stair exit or ladder placeholder** (secondary vertical egress).
- **Optional rooftop or balcony placeholder** (future extension/testing hook).

## Gameplay Placements

- **PlayerStart:** Outside front staging area.
- **Objective item:** In second-floor comms room.
- **Extraction zone:** Outside rear of building.
- **Enemy spawn markers:** 4 to 6 distributed across both floors and approach routes.
- **Patrol route markers:** 2 routes (one first-floor sweep, one second-floor loop).
- **Cover blocks:** Hallways and key room interiors.
- **Doorway choke points:** Major transitions and breach paths.
- **Debug labels:** Added for all major areas for fast callouts and test reporting.

## Test Scenario Goals

The graybox should explicitly support repeatable verification of:

- Sprint / crouch / ADS movement flow.
- Leaning around corners.
- Room clearing cadence and corner checks.
- Stairwell combat transitions.
- Weapon fire, reload, and fire-mode cycling.
- Objective pickup behavior.
- Extraction completion flow.

## Graybox Implementation Notes

- Keep collision simple and readable (blockout-first).
- Prioritize consistent doorway widths and stair dimensions for movement testing.
- Use high-contrast debug text/numbering for area IDs and patrol markers.
- Keep sightlines intentionally mixed (short CQB + a few medium lanes).
- Leave placeholders for future art pass, lighting pass, and encounter scripting.

## Initial Validation Checklist

- [ ] PlayerStart works and faces front approach lane.
- [ ] All three entrances are navigable.
- [ ] Objective in comms room can be reached and interacted with.
- [ ] Rear extraction triggers mission completion.
- [ ] 4–6 enemy spawns initialize correctly.
- [ ] 2 patrol paths execute without navigation breaks.
- [ ] Cover/choke points produce expected tactical pauses.
- [ ] Debug labels visible from expected approach angles.


## Automated Generator Implementation (Phase 1D.1)

The map plan is implemented by `ATU_KillhouseGenerator` (`Source/TheUnit/Public/TU_KillhouseGenerator.h`, `Source/TheUnit/Private/TU_KillhouseGenerator.cpp`).

- Uses `OnConstruction()` to generate and rebuild geometry safely in-editor.
- Clears previously generated components before rebuilding.
- Builds two stories with procedural floors, walls, rooms, stairs, and optional roof.
- Places gameplay placeholders for PlayerStart, Objective, Extraction, Enemy Spawns, and Patrol Points.
- Adds optional area labels via `UTextRenderComponent` for callout readability.
