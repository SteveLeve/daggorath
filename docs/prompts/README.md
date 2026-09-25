# Phase prompts

One prompt per phase, kept so that a phase can be re-run, audited, or handed to a
different agent and produce comparable work.

| Prompt | Phase | State |
|---|---|---|
| [`phase-0-archaeology.md`](phase-0-archaeology.md) | 0 — source archaeology and behavioural reconstruction | complete: `docs/archaeology/phase-0-archaeology-report.md` |
| [`phase-0b-evidence-pack.md`](phase-0b-evidence-pack.md) | 0b — executable evidence pack: fixtures, scheduler spec, headless slice | complete: `docs/archaeology/phase-0b/README.md` |
| [`phase-1-conformance-and-creatures.md`](phase-1-conformance-and-creatures.md) | 1 — ROM conformance harness, then level population and regeneration | captures recorded; close-out is the next prompt |
| [`phase-1-rom-captures.md`](phase-1-rom-captures.md) | 1 (continued) — remaining ROM captures, task log, #13 evidence | recorded at `f669183` in reconciliation §1 |
| [`phase-1-apply-issue-13.md`](phase-1-apply-issue-13.md) | 1 (close-out) — apply the owner's decision on issue #13 | **open**; do not start until that decision is on the issue |

Conventions for writing the next one:

- State the preservation requirement explicitly, and say that it overrides any
  earlier instruction or example that conflicts with it.
- Demand working artifacts, not a plan.
- Name the completion gate, and require actual run output — including the first
  divergence on any mismatch — rather than an assurance that it passed.
- Say what must **not** be built, so scope does not drift forward.
- Require that unverified claims stay explicitly unresolved.
