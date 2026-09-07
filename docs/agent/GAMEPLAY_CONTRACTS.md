# Gameplay Contracts

1. **Raid authority:** `GameMode`/Raid owns the authoritative raid lifecycle, objective/extraction validation, and outcome. `GameState` exposes replicated raid state.
2. **Character boundary:** Character owns player-character state and exposes clean interaction and combat entry points; it does not absorb weapon, inventory, or raid internals.
3. **Reusable health:** `UTUHealthComponent` is the shared regional-health capability for players and eligible AI. Damage delivery belongs to the attacking system; health applies damage and reports consequences.
4. **Canonical weapon:** The project must have one canonical runtime weapon state architecture. `ATU_WeaponBase` is the external weapon actor/API; any subordinate component has a clearly separated mechanical responsibility and cannot be a competing weapon.
5. **Combat callers:** Player and AI request weapon actions through the weapon API. AI never edits ammunition or reload internals directly.
6. **UI boundary:** UI displays state and submits intent. It never decides hits, item ownership, objective completion, extraction, or raid results.
7. **Inventory authority:** Inventory owns item identity, containment, equipment, and ownership. Raid coordinates outcomes without owning individual item internals.
8. **Persistence:** Successful persistence should apply explicit raid deltas, not whole-profile overwrites. Delta application must eventually be authoritative and idempotence-aware.
9. **World boundary:** Generators provide environment geometry and placement/spawn metadata. They do not become god objects controlling AI, objectives, extraction, or raid lifecycle.
10. **Networking path:** Local-first functionality is acceptable, but mutable gameplay must be designed to migrate to server validation and appropriate replication.
11. **No duplicates:** Search for existing types and systems before adding one. Extend or deliberately replace existing ownership; never fork it accidentally.
