# Daggorath Agent Guide

This is an evidence-led reconstruction of the 1983 game. Prefer an explicitly
recorded gap over plausible invented behaviour.

## Read First

Before changing behaviour, read [CLAUDE.md](CLAUDE.md), then the project
[charter](docs/project-instructions.md), the
[clock and scheduler specification](docs/specification/clock-and-scheduler.md),
[Phase 0b reconciliation](docs/archaeology/phase-0b/reconciliation.md), and the
[provenance ledger](docs/provenance/ledger.md). These documents are authoritative;
link to them rather than copying their content into new documentation.

Confirm the active scope in [README.md](README.md) and its corresponding prompt
under [docs/prompts](docs/prompts). Do not start combat, creature movement or
attacks, SDL/mobile UI, player-supplied seeds, or random Original Mode maps unless
the phase scope changes.

## Preservation Rules

- The pinned assembly listing is primary evidence. Treat later ports as
  reference-only until their licence is recorded in the provenance ledger.
- Label behavioural claims `source-proven`, `ROM-observed`, `inferred`, or
  `unresolved`. Preserve original quirks; document intentional deviations in the
  scheduler specification.
- Never invent fixture values or hashes. Do not regenerate a fixture baseline or
  manifest to make a failing comparison pass; record the reason in the applicable
  reconciliation document first.
- Do not add ROMs, emulator captures, manual scans, or third-party port source to
  the repository.

## Architecture

Follow [module boundaries](docs/architecture/module-boundaries.md): dependencies
point inward. `src/core` is headless, deterministic, and links nothing: no SDL,
platform headers, or external I/O. Route input through timestamped keystrokes or
parsed commands. Keep all timing-affecting state and behaviour in `core`.

## Verification

Use the commands documented in [README.md](README.md). For ordinary C++ changes,
run `make build && make test`; run `make verify` whenever fixtures or generated
evidence are involved. Run `make all` for a phase-completion or evidence-pack
change. `make fixtures` requires the pinned source listing and is only for
evidence-derived updates.
