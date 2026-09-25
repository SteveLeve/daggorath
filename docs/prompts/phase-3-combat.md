Carry out **Phase 3: combat and physiology to death** for the Dungeons of
Daggorath preservation project. Read `docs/project-instructions.md`, `CLAUDE.md`,
`docs/planning/roadmap.md`, ADR-0004 and ADR-0008 under `docs/adr/`,
`docs/specification/clock-and-scheduler.md` §§9 and 13,
`docs/specification/creatures.md` and the Phase 2 reconciliation before
starting. Produce working artifacts, not a plan. Do not build item use, magic
beyond the ring gate `PATTK` checks, endings, rendering or UI.

**Preservation requirement.** Hits, misses and damage reproduce the original's
`ATTACK`, `DAMAGE` and `SCAL16` arithmetic at its original byte widths, signed
and unsigned comparisons, overflow and wrap. The player's exertion on attack, the
darkness penalty, faint, recovery and death follow `PATTK`, `COMPLR` and
`HUPDAT`. Combat that "feels wrong" is preserved and classified, never tuned.

**Evidence discipline.** As in every phase: one label per claim, no invented
values, no ROM-observed labels without a capture.

Work in this order.

**1. Specify.** Create `docs/specification/combat-and-items.md` (combat half).
Cover: `PATTK` order of operations (copy held object's values or `EMPHND`,
exertion, sound event, ring charge, then the creature search); `ATTACK`
(quarter-step comparison, RNG byte, reward/penalty, threshold); the dead/unlit
torch 1-in-4 gate; ring bypass; `DAMAGE` magical and physical channels with the
radix-7 factors; `SCAL16`; kill handling including loot drop and the `CMXLND`
decrement in `PATT40`; creature attack from `CMOVE` against the player's shield
if any; player damage into `HUPDAT`; faint (keyboard suspension) and recovery;
`DEATH`. Record the Phase 0 report's "VERIFY exact byte wrapping and signed
flags" as resolved or still open.

**2. Fixtures.** Extract the factor tables, `EMPHND`, and whatever `ATTACK`
reads. Generate exhaustive fixtures from an independent Python implementation of
`SCAL16` and `DAMAGE` for every creature type × starting/available weapon ×
shield combination, and `ATTACK` outcome tables across all 256 RNG bytes for
representative power/damage states. The C++ core must match them.

**3. Implement** `ATTACK` command handling, creature attack (retire D-7), kill,
the matrix decrement, faint and death. A wizard-type kill is detected and emits
`DEFER endgame <type>` per ADR-0008 (new deviation, retired in Phase 5). Loot
drop changes ownership and position only.

**4. Tests and traces.** Regression tests for: attack while fainted (keyboard
suspended), attack in darkness, empty hand, simultaneous player and creature
attack in one jiffy, kill then `CREGEN` then re-entry (count after decrement and
increment), death on the exact jiffy damage exceeds power. Scripts and traces for
a level-0 fight to a kill and a fight to death.

**5. Reconcile** in `docs/archaeology/phase-3/reconciliation.md`; forward
pointer; quirks; capture backlog (hit/miss sequences, faint timing, death
screen timing).

**Do not build:** `USE`, `INCANT`, `REVEAL`, torch burn-down, `CLIMB`, endings,
save, rendering, UI.

**Completion gate.** `make all` actual output; fixture match counts for the
exhaustive tables; first divergence for any mismatch; which combat claims are
source-proven versus unresolved.
