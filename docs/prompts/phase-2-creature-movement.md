Carry out **Phase 2: creature movement** for the Dungeons of Daggorath
preservation project. Read `docs/project-instructions.md`, `CLAUDE.md`,
`docs/planning/roadmap.md`, `docs/adr/0002-scheduler-lap-model.md`,
`docs/adr/0008-deferred-effects-policy.md`,
`docs/specification/clock-and-scheduler.md`, `docs/specification/creatures.md`
and `docs/archaeology/phase-1/reconciliation.md` before starting. Produce working
artifacts and recorded evidence, not a plan. Do not build combat, objects,
rendering or any UI.

**Preservation requirement.** Creatures move by the original `CMOVE` rules
(`CRETUR.ASM`), at the original movement and attack delays from the creature
table, in the original task order, drawing from the one shared RNG in the
original sequence. A creature's first move happens when the original would make
it, not when it would be convenient. If an earlier instruction or example
implies otherwise, follow this requirement.

**Evidence discipline.** Every behavioural claim carries one label:
source-proven, ROM-observed, inferred, or unresolved. No ROM frame has been
captured (`docs/provenance/rom-diff.md`); do not label anything ROM-observed.
Never invent a fixture value, a hash, or a timing.

Work in this order.

**1. Read before writing.** Walk `CMOVE` and every routine it calls. Write
`docs/specification/creatures.md` §§ for: frozen and dead checks, object pickup
(which types, one per action), same-cell detection, row/column alignment and the
traversability test on intervening cells, the random-byte preference order
(forward/left/right vs forward/right/left, the side-first case), back-off, and
requeue delay selection. Cite listing locations. Anything you cannot establish
stays unresolved.

**2. Settle ADR-0002.** Read `SCHED` and every `Q.SCD` return. Choose the lap
policy on source evidence, add a dated Resolution to ADR-0002, and update D-1 and
D-2 in `clock-and-scheduler.md` §13 (retire, relabel or keep, with the reason).
Implement it as one policy in `src/core/scheduler.cpp`.

**3. Queue `CMOVE` at `CBIRTH` and retire D-6.** Creature tasks enter the queue
the listing names with the countdown it names.

**4. Implement `CMOVE` without the attack effect.** Object pickup changes object
ownership only (no burden or inventory rules beyond ownership). When a creature
would attack, follow ADR-0008: perform every source side effect up to the
attack call, emit `DEFER creature-attack <slot>` in the trace, and add deviation
D-7 naming Phase 3 as its retirement. Creature sound selection is a trace event
(type and volume inputs), not audio.

**5. Fixtures and tests.** Extend the extractor with any creature-table fields
movement needs (delays, type flags), with source locations. Add conformance tests
for: each priority branch in isolation; the preference-order distribution over
all 256 random bytes; two or more creatures expiring on the same jiffy as a
keystroke (expected order derived from the listing, labelled); a creature
blocked on all sides; re-entry of a level mid-move (control blocks zeroed by
`NEWLVL`). Add a `dcli` script that runs level 0 for at least 10 simulated
minutes with no input and commit its trace.

**6. Reconcile.** Write `docs/archaeology/phase-2/reconciliation.md` in the Phase 1
shape and append a forward pointer to Phase 1's. Add the captures you would want
to `docs/planning/capture-backlog.md` (at minimum: first creature move after
entry, same-jiffy ties, a 10-minute idle run). Update quirks and ledger.

**Do not build:** damage, player death, the attack branch's effect, object
use, burden changes, rendering, sound output, any UI.

**Completion gate.** Run `make all` and report the actual output: test counts,
failures, the first divergence for any trace mismatch, and `make verify`. Show
that the Phase 0b and Phase 1 traces either still match byte for byte or that
each difference is explained by retired D-6 or the ADR-0002 resolution, with the
explanation recorded in the reconciliation. State which claims are source-proven
and which remain unresolved. Recommend the next slice only after assessing the
gate.
