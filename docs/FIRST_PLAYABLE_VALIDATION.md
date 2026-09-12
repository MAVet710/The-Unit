# First playable validation

**WORKING GAME STATUS: PARTIAL. FIRST PLAYABLE: NOT VALIDATED.**

## Environment and revision

- Session date: 2026-09-12 (provided session date; host Git timestamps can use a different timezone).
- Environment: Linux x86_64, kernel 6.18.35; no discovered Unreal or Windows execution environment.
- Branch: `integration/working-game-v1`.
- Verified base commit: `1c39edbf916e4c6d7ddcf8ccb6c6cdfd8fee4ab6` (`feature/donetsk-reference-map`).
- Source fix commit before publication: `225f437` (cherry-pick of agent/build `7a0bc8c`). API publication can assign a different commit hash while preserving the same source tree. Use the draft PR head SHA to identify the published snapshot.
- Runtime-tested commit: NONE. There is no validated binary revision.
- UE version: project association `5.7`; installed/running UE version: NONE verified.
- Compiler: NONE invoked. No MSVC or UHT result exists.

## Build result

BLOCKED before compiler execution. Explicit world/type includes were added in three files; they are not a compiler-confirmed build fix. Post-integration tool discovery exited 2 after finding no Build.bat, UnrealEditor-Cmd.exe, UnrealBuildTool, cl.exe, pwsh or wine. See BUILD_VALIDATION.md.

## Automation summary

| Suite | Source registrations | Executed | Runtime result |
| --- | ---: | ---: | --- |
| TheUnit.Combat.* | 12 | 0 | NOT RUN |
| TheUnit.CommandCenter.* | 4 | 0 | NOT RUN |
| TheUnit.Hideout.EndToEnd.* | 4 | 0 | NOT RUN |
| TheUnit.MX50.* | 3 | 0 | NOT RUN |
| TheUnit.Maps.Donetsk.* | 3 | 0 | NOT RUN |
| TheUnit.Operator.* | 0 | 0 | No registrations under this prefix |
| TheUnit.FPV.* | 0 | 0 | Separate FPV v8 has 2; not integrated |
| Total integrated source | 26 | 0 | NOT RUN |

Existing EndToEnd-named tests check defaults, schema and class wiring; their names are not evidence of a played mission. Source whitespace/header checks and the Python repeated-INI-key reproduction ran. These do not execute Unreal.

## Required runtime tests

| Test | Result | Source observation / unresolved prerequisite |
| --- | --- | --- |
| CommandCenter | NOT RUN | Generator/game mode exist; .umap absent |
| Player | NOT RUN | Default modular → armed → operator hierarchy exists |
| Armory | NOT RUN | Selection widgets/components exist |
| Cage | NOT RUN | No populated authored/default gear inventory found |
| Range | NOT RUN | Target damage/reset code exists |
| Weapons/melee | NOT RUN | Classes/input exist; firing/switching/damage unverified |
| Equipment/armor | NOT RUN | Regional armor path exists; assets/death loop incomplete |
| MX50 | NOT RUN | Physical placeholder/page UI exists; persistent page/map/feed state incomplete |
| Hideout persistence | NOT RUN | Save schema/subsystem exists; no restart test |
| Killhouse | NOT RUN | Geometry and extraction bootstrap exist; map/AI/objective incomplete |
| Enemy AI | NOT RUN | Required implementation missing |
| Objective | NOT RUN | Objective base is empty |
| Extraction | NOT RUN | Timer source exists; objective/player eligibility missing |
| Save/load | NOT RUN | Loadout/upgrades/active mission/count fields exist; cross-restart behavior unverified |
| Donetsk | NOT RUN | Civilian district/Artema 60 generators exist; no map/runtime traversal |
| FPV | NOT RUN | v8 inspected, not merged; infantry/tablet handoff requires coordination |
| Development package | NOT RUN | No UAT execution or artifact |
| Shipping package | NOT RUN | No UAT execution or artifact |
| Packaged executable | NOT RUN | No executable launched |

## Performance baseline

| Map | FPS | Game thread | Render thread | GPU | Actor count | Component count / duplication |
| --- | --- | --- | --- | --- | --- | --- |
| CommandCenter | Not measured | Not measured | Not measured | Not measured | Not measured | Not measured |
| Killhouse | Not measured | Not measured | Not measured | Not measured | Not measured | Not measured |
| Donetsk | Not measured | Not measured | Not measured | Not measured | Not measured | Not measured |

No source estimate is substituted for a measured baseline.

## Exact packaged playthrough record

All steps below are **NOT RUN**. Completed steps: **0/42**.

1. Launch game.
2. Spawn in CommandCenter.
3. Walk to Armory.
4. Equip TU-556, G34 and OTF.
5. Walk to Cage.
6. Equip available helmet/armor/rig.
7. Walk to Range.
8. Fire primary.
9. Reload.
10. Switch secondary.
11. Fire secondary.
12. Draw melee.
13. Attack compatible target.
14. Stow melee.
15. Raise MX50 with T.
16. Switch MX50 pages.
17. Stow MX50.
18. Walk to Briefing.
19. Press F.
20. Confirm Killhouse operation loads.
21. Review mission/map/intel/loadout.
22. Go to deployment station.
23. Deploy.
24. Spawn in Killhouse.
25. Engage enemies.
26. Complete objective.
27. Reach extraction.
28. Begin extraction countdown.
29. Leave zone once.
30. Verify countdown cancels.
31. Re-enter.
32. Complete extraction.
33. Return to CommandCenter.
34. Verify loadout persisted.
35. Verify hideout state persisted.
36. Stop game.
37. Restart game.
38. Verify persistence again.
39. Load Donetsk.
40. Walk through district.
41. Verify Artema 60 and primary architecture areas.
42. Return/quit cleanly.

## Known limitations and remaining blockers

See KNOWN_LIMITATIONS.md for per-blocker evidence, affected source, fixes pending, and retest status. Milestone A must execute on an accessible Windows UE 5.7 machine before downstream gameplay integration resumes. No source architecture was replaced, no feature branch was overwritten, no FPV branch was blindly merged, and nothing was merged into main.
