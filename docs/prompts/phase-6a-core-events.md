Carry out **Phase 6a: the core event contract** for the Dungeons of Daggorath
preservation project. This is a small phase run after Phase 2 and before
Phase 3. Read `docs/adr/0004-presentation-contract.md`,
`docs/architecture/module-boundaries.md`, `CLAUDE.md` and the Phase 2
reconciliation first. Produce working code and tests, not a plan.

**Hypotheses, not evidence.** Behaviour this prompt names (priorities,
thresholds, effects, routine roles) comes from earlier reports and is a target
to verify against the listing. Cite the listing, never this prompt.

**Hypotheses, not evidence.** Behaviour this prompt names (priorities,
thresholds, effects, routine roles) comes from earlier reports and is a target
to verify against the listing. Cite the listing, never this prompt.

**Preservation requirement.** Adding the event stream changes no simulation
behaviour: no RNG draw, task order, countdown or trace line other than the ones
this phase deliberately renames changes. Blocking durations stay 0 with
`duration_known = false` wherever the specification says unresolved (D-4).

Work in this order.

1. **Confirm ownership from the listing.** For each event kind in ADR-0004
   rule 1, cite where the simulation emits it. Move display-only kinds to the
   presentation list. Record the result as the ADR's dated Resolution for 6a.
2. **Define** `CoreEvent` in `src/core/include/daggorath/` with jiffy and
   scheduler-position stamps. No presentation header, no platform header.
3. **Emit** events from the paths that exist after Phase 2 (creature sound
   selection, MOVE/TURN animation start, heartbeat toggle, text output) and
   replace the corresponding ad hoc trace lines with a stable printed form.
4. **Tests.** Event order for a same-jiffy creature expiry and keystroke; the
   Phase 0b and Phase 2 traces differ only by the renamed lines, and each rename
   is listed in the reconciliation.

**Do not build:** `RenderState`, the viewer, draw lists, any presentation code,
audio or SDL.

**Completion gate.** `make all` output; `boundary-checker` report; the trace
diff with every changed line accounted for; ADR-0004 6a Resolution present.
