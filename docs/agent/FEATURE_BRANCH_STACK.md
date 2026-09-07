# Feature Branch Stack

These branches contain staged UE C++ work that is intentionally **not merged to `main` until an Unreal Engine 5.7 build and automation pass succeeds**. Do not rediscover or rewrite these systems before validating the existing stack.

## Modular weapon stack

Validate the latest branch first; it contains all earlier weapon commits in order:

1. `agent/tu-005-modular-weapon-data` — `048331d026e3a187055b21ccefca71af992e8160`
2. `agent/tu-006-weapon-compatibility` — `9c748e523c11f8e63dac742c0309839696791061`
3. `agent/tu-007-fire-control-data` — `dc1a155e5685b458d30e7e9e4b41165e5cc26a18`
4. `agent/tu-008-weapon-build-resolver` — `d697ace76802e404167dcc85a691bdf8c8a31940`
5. `agent/tu-009-runtime-fire-control` — `b02cd1143fc293234a4df21f9a01898f92e81100`
6. `agent/tu-010-weapon-definition-catalog` — `404153798d4423fd05c92010fbf354c596f895d9`
7. `agent/tu-011-catalog-runtime-configuration` — `f8023accc202ba4ade6874680805cea6b71182b1`
8. `agent/tu-012-weapon-build-editor` — `f9d6dedbbbaa6dd6355f00406067007eca218acb`
9. `agent/tu-013-trigger-as-part` — `a0bdda2848924337282d45c2cc555d1b8b696f50`
10. `agent/tu-014-catalog-authoritative-edits` — `8fb5aecde4441402deb75ad462926a356e9dabc5`
11. `agent/tu-015-weapon-catalog-validation` — `a9a62a8385a5676114908102aa20625ee61b8e07`
12. `agent/tu-016-weapon-instance-state` — `12903180bdb92a75a8b4a341a9729e5237860cab`
13. `agent/tu-017-runtime-instance-hydration` — `8ddf9f454d3ae8b7556067fae61f6848939866c0`

Latest validation target: `agent/tu-017-runtime-instance-hydration`.

The stack implements data-driven platforms/parts, compatibility and derived stats, independent fire-control and trigger parts, definition catalogs, catalog-authoritative build editing, catalog validation, persistent instance identity/state, and runtime hydration/export.

## Character stack

1. `agent/wave1-operator-health-integration` — `9afa671280accc01e2e27360d78e53f2413b2414`
2. `agent/wave1-operator-death-lifecycle` — `91b09c562ec03c6ec63c19abf01762e40416ee4a`

Latest validation target: `agent/wave1-operator-death-lifecycle`.

## Validation

From a local checkout with UE 5.7 installed, run:

```powershell
.\Scripts\ValidateTheUnit.ps1
```

Set `UE_ENGINE_ROOT` or pass `-EngineRoot` if UE is installed elsewhere. The script builds `TheUnitEditor Win64 Development` and runs all automation tests under `TheUnit.`.

The weapon stack should include the existing ownership regression plus focused tests for modular definitions, compatibility/build rules, fire-control, resolver/catalog behavior, runtime configuration, build editing, trigger parts, catalog authority/validation, instance state, and runtime instance hydration.

The character stack should include operator health integration and death lifecycle tests in addition to the existing main regressions.

Do not merge either stack to `main` until its branch builds and its automation suite passes with no regressions caused by the staged changes.
