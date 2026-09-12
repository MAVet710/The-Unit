# Working Game V1 limitations and blockers

This snapshot is not a first playable. Findings below distinguish inspected source from executed engine behavior. No gameplay or packaging PASS is claimed.

## Environment gate

BLOCKER: Windows UE 5.7 execution unavailable.

SYSTEM: Build/UHT, maps, runtime, automation, performance, packaging.

ERROR: Executable discovery found no Build.bat, UnrealEditor-Cmd.exe, UnrealBuildTool, cl.exe, pwsh or wine. Post-integration preflight exited 2 before compilation.

LOG: BUILD_VALIDATION.md environment evidence. No Unreal build log was generated.

ROOT CAUSE: Available host is Linux with no discovered UE installation or connected Windows execution runner.

FIX: Three explicit type/world includes were added as source hygiene; these do not resolve the missing execution environment.

RETEST: UE not run. All later runtime gates remain blocked.

## Source gaps requiring implementation after gate A

| BLOCKER | SYSTEM | ERROR / evidence | LOG / source | ROOT CAUSE | FIX | RETEST |
| --- | --- | --- | --- | --- | --- | --- |
| No serialized maps | Maps/cook | No tracked .umap files or DefaultEngine.ini | Tools/Unreal/create_end_to_end_maps.py | Generator has not produced committed assets | Pending actual Unreal generation; no fake assets created | NOT RUN |
| Repeated INI entries lost | Map helper | In-memory ConfigParser reproduction reduced 3 +MapsToCook values to 1 | create_end_to_end_maps.py, _write_startup_map_config | General INI parser collapses repeated Unreal keys while rewriting file | Pending after build gate; preserve unrelated INI text/array entries | Python reproduction only; UE NOT RUN |
| Missing scene setup | Maps/navigation | No light or navigation volume creation found in inspected bootstrap/source | Map bootstrap and native generators | Maps rely on nearly empty worlds and native geometry | Pending lighting and navigation setup/validation | NOT RUN |
| Missing enemy AI | PvE | No reusable enemy character/controller/perception or spawn implementation | Source/TheUnit inventory; TU_TrainingMissionGameMode.cpp | Current bootstrap creates geometry/extraction, no opponents | Pending minimum finite-spawn PvE loop | NOT RUN |
| Empty objective actor | Missions | ATU_ObjectiveBase has no lifecycle implementation | Source/TheUnit/Public/TU_ObjectiveBase.h | No deterministic success authority | Pending objective state and eligibility | NOT RUN |
| Extraction bypasses objectives | Extraction | Any APawn can start timer; completion flag forwarded directly | TU_ExtractionZone.cpp, HandleBeginOverlap/ExtractNow | No objective/death/player eligibility checks | Pending lifecycle integration | NOT RUN |
| Incomplete death lifecycle | Health/combat | Death broadcast has no incapacitation/control/combat listener found | TUHealthComponent.cpp and TU_ModularOperatorCharacter.cpp | Damage notification not connected to mission/player shutdown | Pending coherent death flow | NOT RUN |
| Empty default Cage inventory | Equipment | AvailableGear has no authored assets/fallback definitions supplied | TUOperatorEquipmentComponent and Content inventory | Definitions must be populated | Pending placeholder DataAssets using existing architecture | NOT RUN |
| Incomplete persistence | Save/MX50 | No tablet page/map/markers/video fields or per-mission completion identities | Source/TheUnit/Public/TUHideoutSaveGame.h | Current schema only covers a subset | Pending schema/lifecycle extension | NOT RUN |
| Missing gameplay HUD | Usability | No required health/ammo/objective/extraction HUD implementation found | Source/TheUnit inventory | Station/tablet UI does not cover gameplay HUD | Pending minimal HUD | NOT RUN |
| Incomplete feedback | Presentation | No complete required audio/VFX feedback loop found | Firearm/target/UI source | Existing delegates/recoil/reset are partial | Pending engine-safe/project-owned placeholders | NOT RUN |
| FPV remains separate | Drone | v8 operator possession does not coordinate current combat/UI or MX50 | origin/feature/fpv-flight-core-v8 | Older parallel infantry implementation | Pending selective flight components, input/modules and possession handoff; no blind branch merge | NOT RUN |
| No cooked build | Packaging | No Development/Shipping executable exists | PACKAGED_BUILD_VALIDATION.md | UAT/Windows runtime and maps unavailable | Pending all prerequisite gates | NOT RUN |

Killhouse extraction geometry also needs runtime inspection: trigger center Y=1950 cm is beyond floor rear edge Y=1940 cm, with overlap half-extent 220 cm. This is a traversal/eligibility risk, not a demonstrated runtime failure.

Donetsk civilian reference sources and architecture were preserved. No current military locations or operational intelligence were added. No paid dependencies, services, assets or cloud infrastructure were introduced.
