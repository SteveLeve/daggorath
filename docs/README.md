# Documentation index

## Start here

| Document | What it is |
|---|---|
| [`project-instructions.md`](project-instructions.md) | the charter: mission, principles, development sequence, licensing and agent rules |
| [`../README.md`](../README.md) | project state, layout and build |

## Living documents

These are updated as evidence arrives.

| Document | What it is |
|---|---|
| [`specification/clock-and-scheduler.md`](specification/clock-and-scheduler.md) | jiffy-by-jiffy clock, queue and dispatch behaviour; every rule labelled by evidence class; deviations listed in §13 |
| [`architecture/module-boundaries.md`](architecture/module-boundaries.md) | the modules, what each may depend on, and why |
| [`provenance/ledger.md`](provenance/ledger.md) | pinned commits, per-file hashes, tool versions, rights position, and which extracted data is copied rather than computed |
| [`licensing/README.md`](licensing/README.md) | the licensing questions that must be answered before distribution |

## Archaeology reports

Dated deliverables. They record what was known at the time and are not rewritten;
corrections appear in the next report.

| Document | What it is |
|---|---|
| [`archaeology/phase-0-archaeology-report.md`](archaeology/phase-0-archaeology-report.md) | Phase 0 architectural reconstruction, with a Phase 0b reconciliation section appended |
| [`archaeology/phase-0b/README.md`](archaeology/phase-0b/README.md) | Phase 0b evidence pack: fixtures, traces, gate report |
| [`archaeology/phase-0b/reconciliation.md`](archaeology/phase-0b/reconciliation.md) | each investigated `VERIFY` item: evidence, result, confidence, what is still open |
| [`archaeology/phase-0b/fixtures/`](archaeology/phase-0b/fixtures/) | machine-readable fixtures with a hash per file |
| [`archaeology/phase-0b/traces/`](archaeology/phase-0b/traces/) | keystroke scripts, their outputs, and the ROM capture procedure |

## Prompts

[`prompts/`](prompts/README.md) holds the working prompt for each phase, so a
phase can be re-run or audited later.

## Planning and decisions

[`planning/`](planning/README.md) holds the roadmap for Phases 2–11, the ROM
capture backlog and the review checklist. [`adr/`](adr/README.md) records
decisions that constrain more than one phase.
