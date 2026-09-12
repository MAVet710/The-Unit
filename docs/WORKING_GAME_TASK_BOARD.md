# Working Game V1 coordination

Status: PARTIAL source preparation; NOT a working playable build. Milestone A is blocked.

Repository fetched with `git fetch --all --prune`. Clean fresh checkout; no existing local changes discarded.
Latest remote branch by commit date was `feature/donetsk-reference-map`, SHA `1c39edbf916e4c6d7ddcf8ccb6c6cdfd8fee4ab6`.
It contains the hideout, equipment and MX50 stack. FPV v8 is separate and was not merged.
Integration branch: `integration/working-game-v1`. PR base: `feature/donetsk-reference-map`. Never merge main.

Four independent agents performed bounded prerequisite work in separate git worktrees. Other agents were not started because the user explicitly requires successful editor compilation before downstream integration. Source inspections are not runtime tests.

| TASK | OWNER | BRANCH | DEPENDENCIES | FILES / SYSTEMS OWNED | STATUS | BLOCKERS | TEST RESULT | MERGED INTO INTEGRATION? |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Coordination, integration, final evidence | Lead / Agent 0 | integration/working-game-v1 | All gates | This board, FIRST_PLAYABLE_VALIDATION.md, KNOWN_LIMITATIONS.md | Blocked at A | No Windows UE runtime | Environment preflight blocked | Direct commits |
| UE build prerequisites | Agent 1 / build | agent/build | Windows UE 5.7 | TUCalloutManagerComponent.cpp, TU_ExtractionZone.cpp includes, TU_ArmedOperatorCharacter.h include, BUILD_VALIDATION.md | Three include fixes integrated | Compiler/UHT unavailable | Source checks only; UE NOT RUN | Yes, focused cherry-pick |
| Map bootstrap assessment | Agent 2 / maps | agent/maps | A | Tools/Unreal scripts, map config and GameModes; read only | Assessment complete; implementation gated | No engine or serialized maps | INI helper reproduction only; UE NOT RUN | No changes |
| Player | Agent 3 / deferred | agent/player (reserved) | A, maps | Operator hierarchy, input/controller | Not started | A | NOT RUN | No |
| Combat | Agent 4 / deferred | agent/combat (reserved) | A, player | Firearm/melee runtime | Not started | A | NOT RUN | No |
| Equipment/armor | Agent 5 / deferred | agent/equipment (reserved) | A, combat | Equipment, armor, regional health | Not started | A | NOT RUN | No |
| Hideout | Agent 6 / deferred | agent/hideout (reserved) | A, lifecycle | Hideout stations, armory/cage/range | Not started | A | NOT RUN | No |
| MX50 | Agent 7 / deferred | agent/mx50 (reserved) | A, lifecycle | Tablet state and widgets | Not started | A | NOT RUN | No |
| Mission lifecycle | Agent 8 / deferred | agent/mission-lifecycle (reserved) | A, equipment | Save, travel, extraction, objective eligibility | Not started | A | NOT RUN | No |
| Enemy AI | Agent 9 / deferred | agent/ai (reserved) | A, player, damage | New reusable fictional PvE AI | Not started | A; no current AI | NOT RUN | No |
| Killhouse | Agent 10 / deferred | agent/killhouse (reserved) | A, AI, lifecycle | Mission bootstrap, objectives, geometry | Not started | A | NOT RUN | No |
| Donetsk | Agent 11 / deferred | agent/donetsk (reserved) | A, maps | Civilian district generator | Not started | A | NOT RUN | No |
| FPV | Agent 12 / deferred | agent/fpv (reserved) | A, infantry, MX50 | Selective FPV v8 integration | Not started; QA inspected source | A; handoff integration absent | NOT RUN | No |
| HUD | Agent 13 / deferred | agent/ui (reserved) | A, mission state | Gameplay HUD | Not started | A; HUD absent | NOT RUN | No |
| Feedback | Agent 14 / deferred | agent/presentation (reserved) | A, gameplay events | Legal placeholder audio/VFX | Not started | A | NOT RUN | No |
| Regression assessment | Agent 15 / qa | agent/qa | A for execution | Read-only system and test inventory | Assessment complete; execution gated | Unreal unavailable | 26 registrations, 0 executed | No changes |
| Packaging prerequisites | Agent 16 / packaging | agent/packaging | A through I | PACKAGED_BUILD_VALIDATION.md only | Evidence report complete; packaging gated | No UAT/maps/Windows runtime | NOT RUN | Documentation cherry-picked only |

## Integration boundary

Only the build include corrections changed source. After that integration, executable discovery returned no Build.bat, UnrealEditor-Cmd.exe, UnrealBuildTool, cl.exe, pwsh or wine. Preflight exited 2; compilation never started. No downstream gameplay source was integrated. The packaging report is documentation, not a package milestone.

No active agent owns overlapping files. Future lifecycle work must preserve the explicit Engine/World.h include added to extraction. Reserved branches are not claimed to exist. Do not overwrite existing branches when resuming.

Shell `git push` failed for missing HTTPS credentials; the connected GitHub account created the integration branch. Publication uses non-forced GitHub ref updates and preserves existing branch heads. Final published SHA is the PR head; local source commits may have different metadata hashes when transferred through the API.

## Dependency order on resumption

Compile A first. Then maps → player → combat → equipment → mission lifecycle → hideout → MX50 → AI → Killhouse → Donetsk → optional FPV → HUD → feedback → QA → packaging. Compile after each source integration; use runtime evidence at the tested commit for every gameplay gate. Stop at failures. The requested 42-step packaged playthrough remains 0/42.
