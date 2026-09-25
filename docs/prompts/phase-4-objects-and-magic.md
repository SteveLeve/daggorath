Carry out **Phase 4: objects, inventory, torches, magic and vertical travel** for
the Dungeons of Daggorath preservation project. Read the charter, `CLAUDE.md`,
`docs/planning/roadmap.md`, ADR-0006 and ADR-0008,
`docs/specification/combat-and-items.md`, `docs/specification/dungeon-and-rng.md`
and the Phase 3 reconciliation first. Produce working artifacts, not a plan.

**Hypotheses, not evidence.** Behaviour this prompt names (priorities,
thresholds, effects, routine roles) comes from earlier reports and is a target
to verify against the listing. Cite the listing, never this prompt.

**Preservation requirement.** Every object behaves as the listing's handlers
make it behave: `PGET` (GET, DROP, STOW, PULL), `PEXAM`, `PREVEA`, `PUSE`,
`PINCAN`, `PCLIMB`, `BURNER`, `OBIRTH`. Generic parameters until reveal,
burden, the absence of an explicit bag cap if the source confirms it, torch
lifetimes and the dead-torch threshold, flask and scroll effects, ring
transformations by full incantation, and climbing only where a vertical feature
permits it. Player-facing words come from `tokens.json`, never from item labels
in comments or from the manual. A port that allowed ascending holes did so in
error (Phase 0 report §17); Original Mode does not.

Work in this order.

1. **Specify** the items half of `combat-and-items.md` and the vertical-travel
   section of `dungeon-and-rng.md` (resolve the Phase 0b `VFTTAB` mapping open
   item using the `NEWLVL` `VFTPTR` indexes Phase 1 recorded). Settle the
   manual's bare `CLIMB` against the source and label it.
2. **Implement** the commands above. `EXAMINE` and the map scrolls produce state
   and a display-mode change event (ADR-0004 or a named trace line), not a
   drawing. `CLIMB` performs a player level change through the existing
   `NEWLVL` path. A final-ring incantation and the wizard endings emit `DEFER`
   per ADR-0008, retired in Phase 5.
3. **Torches:** `BURNER` on its one-minute schedule, light level as state for
   the later viewer, and the torch's effect on the Phase 3 darkness gate.
4. **Tests:** torch at the exact minute boundary and at the threshold; reveal
   then examine; each flask; each scroll revealed and unrevealed; each ring word
   held and not held; climbing up and down at each feature and where none
   exists; burden after each GET/DROP; the full starting inventory against
   `initial-state.json`.
5. **Coverage check:** add a test that every verb in `tokens.json` reaches a
   handler; only `ZSAVE`/`ZLOAD` may still report `UNIMPLEMENTED`.
6. **Reconcile** in `docs/archaeology/phase-4/reconciliation.md`, forward
   pointer, quirks, ledger, capture backlog.

**Do not build:** endings, save/load, map drawing, rendering, UI.

**Completion gate.** `make all` actual output, first divergence on any mismatch,
the verb coverage test result, labels summary.
