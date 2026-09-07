# Agent Rules

1. Read `NEXT_TASK.md` first.
2. Read only its **Required Context** unless blocked.
3. Search for a symbol before opening broad directories.
4. Never recursively reread `Content/` for a C++-only task.
5. Do not reread documents already consumed in the current task.
6. Use the smallest model/context adequate for mechanical work.
7. Escalate to high reasoning only for architecture, multiplayer, persistence, complex Unreal failures, or difficult cross-system bugs.
8. Review the task, relevant contracts, changed files, compiler/test output, and `git diff`—not the whole repository.
9. One agent should primarily own each subsystem.
10. Explicitly declare cross-subsystem changes.
11. Do not silently fix unrelated issues.
12. Update `CURRENT_STATE.md` only for material state changes.
13. Keep documentation concise.
14. Prefer small tasks and pull requests.
