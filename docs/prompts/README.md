# Phase prompts

One prompt per phase, kept so that a phase can be re-run, audited, or handed to a
different agent and produce comparable work.

| Prompt | Phase | State |
|---|---|---|
| [`phase-0-archaeology.md`](phase-0-archaeology.md) | 0 — source archaeology and behavioural reconstruction | complete: `docs/archaeology/phase-0-archaeology-report.md` |
| [`phase-0b-evidence-pack.md`](phase-0b-evidence-pack.md) | 0b — executable evidence pack: fixtures, scheduler spec, headless slice | complete: `docs/archaeology/phase-0b/README.md` |
| [`phase-1-conformance-and-creatures.md`](phase-1-conformance-and-creatures.md) | 1 — ROM conformance harness, then level population and regeneration | **open** |
| [`phase-2-creature-movement.md`](phase-2-creature-movement.md) | 2 — `CMOVE` without attack; scheduler lap model (ADR-0002) | planned |
| [`phase-6a-core-events.md`](phase-6a-core-events.md) | 6a — core-owned `CoreEvent` contract (ADR-0004), run after 2 and before 3 | planned |
| [`phase-3-combat.md`](phase-3-combat.md) | 3 — `PATTK`, `ATTACK`, `DAMAGE`, creature attack, faint, death | planned |
| [`phase-4-objects-and-magic.md`](phase-4-objects-and-magic.md) | 4 — objects, inventory, torches, magic, `CLIMB` | planned |
| [`phase-5-progression-and-save.md`](phase-5-progression-and-save.md) | 5 — endings, `ZSAVE`/`ZLOAD`; headless game complete | planned |
| [`phase-6-presentation-state.md`](phase-6-presentation-state.md) | 6 — render state, vector data, sound events (ADR-0004) | planned |
| [`phase-7-desktop-sdl.md`](phase-7-desktop-sdl.md) | 7 — SDL3 desktop app, rasteriser, audio | planned |
| [`phase-8-touch-input.md`](phase-8-touch-input.md) | 8 — touch adapters emitting keystrokes | planned |
| [`phase-9-mobile-packaging.md`](phase-9-mobile-packaging.md) | 9 — Android and iOS shells (licensing precondition) | planned |
| [`track-r-rom-observation.md`](track-r-rom-observation.md) | R — ROM captures, run whenever system ROMs are legally available (ADR-0003) | blocked on firmware |

Conventions for writing the next one:

- State the preservation requirement explicitly, and say that it overrides any
  earlier instruction or example that conflicts with it.
- Demand working artifacts, not a plan.
- Name the completion gate, and require actual run output — including the first
  divergence on any mismatch — rather than an assurance that it passed.
- Say what must **not** be built, so scope does not drift forward.
- Require that unverified claims stay explicitly unresolved.

Planned prompts were written ahead of their phase (2026-09-25, see
[`../planning/roadmap.md`](../planning/roadmap.md)). Before opening one, re-read
it against the previous phase's reconciliation and amend it where the evidence
moved; record the amendment in the new phase's reconciliation. A review agent
checks each phase with [`../planning/review-checklist.md`](../planning/review-checklist.md).

Behaviour named in a planned prompt (routine priorities, thresholds, effects)
is a **hypothesis to verify** against the listing, taken from earlier reports.
The prompt is not evidence; cite the listing, not the prompt.
