# Architecture decision records

One file per decision that constrains more than one phase. Format: context,
decision, consequences, and the evidence or open question it rests on.

Status values: **Proposed** (to be settled in the named phase), **Accepted**,
**Superseded by ADR-NNNN**. A phase that settles a Proposed ADR edits its status
and adds a dated "Resolution" section; it does not rewrite the original text.

| ADR | Title | Status | Settled in |
|---|---|---|---|
| [0001](0001-phase-sequence-after-phase-1.md) | Phase sequence after Phase 1 | Accepted | — |
| [0002](0002-scheduler-lap-model.md) | Scheduler lap model once creature tasks run (D-1, D-2) | Proposed | Phase 2 |
| [0003](0003-rom-observation-track.md) | ROM observation as a parallel track, not a phase gate | Accepted | — |
| [0004](0004-presentation-contract.md) | Presentation contract: render state and ordered events | Proposed | Phase 6 (6a after Phase 2) |
| [0005](0005-save-state-format.md) | Save/load semantics and modern save format | Proposed | Phase 5 |
| [0006](0006-historical-data-assets.md) | Extracted historical data: geometry, sound, tables | Accepted | — |
| [0007](0007-mode-separation.md) | Original Mode isolation and extension points | Accepted | — |
| [0008](0008-deferred-effects-policy.md) | How a phase stubs behaviour owned by a later phase | Accepted | — |
