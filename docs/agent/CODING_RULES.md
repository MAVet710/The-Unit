# Coding Rules

- Follow Unreal Engine C++ conventions and preserve the module's existing naming/style.
- Keep public APIs small, Blueprint-friendly where useful, and explicit about ownership and authority.
- Reuse enums and definitions in `TheUnitTypes.h`; do not create parallel gameplay data types without an architectural reason.
- Prefer components for reusable capability and actors/controllers for world identity or orchestration.
- Avoid duplicated mutable state. Name the single authority for ammunition, health, inventory ownership, and raid outcome.
- Do not add `Tick` when events or timers suffice.
- Do not add module dependencies until used by a concrete implementation.
- Treat client/UI calls as intent. Validate authoritative gameplay on the server as networking is introduced.
- Keep changes scoped to the active task. Declare necessary cross-subsystem edits; do not silently fix unrelated issues.
- Preserve useful Blueprint compatibility when consolidating APIs, and report compatibility risks.
- Add focused tests alongside testable behavior. Run the narrowest available compile/test and inspect the final diff.
- Comments should explain ownership, constraints, or non-obvious decisions—not restate code.
