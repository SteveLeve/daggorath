Carry out **Phase 1: ROM conformance harness, then level population and creature
regeneration** for the Dungeons of Daggorath preservation project. Read
`docs/project-instructions.md`, `CLAUDE.md`,
`docs/specification/clock-and-scheduler.md`,
`docs/archaeology/phase-0b/reconciliation.md` and `docs/provenance/ledger.md`
before starting. Produce working artifacts and recorded evidence, not a plan. Do
not build combat, and do not build any mobile or SDL UI.

**Preservation requirement.** Original Mode reproduces the original five fixed
maps, the original initial creature populations from `CMTTAB`, the original
five-minute regeneration rule, and the original event order. Maze construction is
seeded per level and the clock-dependent RNG advance happens only after the maze
and all 115 doors are complete, so creature placement varies with entry time while
the map does not: preserve both halves of that. A player-supplied seed, newly
generated maps, or any independent spawn schedule belong only to a separate future
mode. If an older instruction or example implies otherwise, follow this
requirement.

**Evidence discipline.** Every behavioural claim carries one label: source-proven,
ROM-observed, inferred, or unresolved. Phase 0b produced no ROM-observed rules. Do
not promote an inferred rule without recording the evidence that promoted it, and
leave what you cannot establish explicitly unresolved. Never invent a fixture value
or a hash, and never regenerate a baseline or a manifest to make a comparison pass.

Work in this order.

**1. Close the ROM equivalence blocker, or record precisely why it stays open.**
Assemble the pinned listing with `lwasm`, hash the resulting cartridge image, and
diff it against a retail ROM. Write the result to `docs/provenance/rom-diff.md`:
tool versions, the ROM's hash and rights basis, the byte differences, and what each
difference means for the claims in `reconciliation.md`. Add the ROM and emulator
rows to `docs/provenance/ledger.md` §1 before any capture is used as evidence. If
a ROM or `lwasm` cannot be obtained, say so in one place, keep every ROM-dependent
conclusion marked unverified, and continue — do not restate the obstacle in each
document, and do not present source-derived output as ROM validation.

**2. Build the emulator harness under `tools/rom/`.** Implement to the procedure
already specified in `docs/archaeology/phase-0b/traces/README.md` §3 rather than
inventing a second one: sample `tools/rom/watchlist.tsv` at 60 Hz, inject
keystrokes from the existing `<jiffy> <KEY>` script format, and emit the existing
trace format so a ROM trace and a `dcli` trace diff line by line. Provide
`trace_diff.py`, which reports the **first** divergence with its jiffy and both
sides' values. Keep capture instructions repeatable and keep raw observations
separate from interpretation. No ROM image or capture enters the repository.

**3. Replay the Phase 0b scripts against the ROM and report what you find.** Take
the five existing scripts first, then the priority captures in that same §3: level
entry at several clock times, level re-entry, the five-minute regeneration
boundary, simultaneous expirations, parser edge cases, and MOVE/TURN/LOOK timing.
For each: the first divergence, or a statement that there is none. Resolve the
Phase 0b deviations D-1 through D-5 where a capture can settle them, and measure
the animation and sound durations D-4 leaves open.

**4. Extend the core with level population, not creature behaviour.** Implement
`NEWLVL`'s creature instantiation from `CMTTAB` and its object distribution, the
creature control block state the specification names, and `CREGEN`'s five-minute
task. Answer the standing blocker: how and when an incremented matrix entry becomes
a live creature, including on returning to a level. Add fixtures for the initial
population of each level and for the creature and object records created at entry,
each with a source location and an extraction method, and extend
`tools/verify_manifest.py` coverage accordingly. Do not implement creature movement
or attacks; do not let the core grow to hide an unresolved timing difference.

**5. Write the specification sections the evidence now supports.** Create
`docs/specification/dungeon-and-rng.md` and
`docs/specification/commands-and-parser.md` from the proven material in
`reconciliation.md` §1.2 and §1.3 so the behaviour lives in the living
specification rather than only in a phase report, and create
`docs/specification/creatures.md` for what this phase establishes. Update
`clock-and-scheduler.md` where a capture changes a label or retires a deviation.

**6. Reconcile.** Write `docs/archaeology/phase-1/reconciliation.md` in the shape of
the Phase 0b one: per item, the evidence, the result, the confidence, and any
source-versus-ROM difference. Append a reconciliation section to the Phase 0b pack
pointing forward, in the way Phase 0b appended one to the Phase 0 report, rather
than editing the dated report's body. Update the quirks ledger and the provenance
ledger.

**Completion gate.** Provide the harness and its scripts, the ROM diff document or
a recorded obstacle, the trace comparisons with first divergences, the extended
core with its fixtures and tests, the new specification sections, and the
reconciliation document. Run `make all` and report the actual output: test counts,
failures, the first divergence for any mismatch, and `make verify` results. State
plainly which claims are now ROM-verified and which remain source-derived.
Recommend the next slice only after assessing this gate. Do not expand into combat
or mobile work during Phase 1.
