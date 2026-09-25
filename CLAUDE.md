# Agent working rules

This repository reconstructs a 1983 game from evidence. The rules below exist
because a plausible-looking guess is worse than an admitted gap.

## Read before changing behaviour

1. `docs/project-instructions.md` — the charter. It wins over anything here.
2. `docs/specification/clock-and-scheduler.md` — the timing model, with every rule
   labelled source-proven / ROM-observed / inferred / unresolved.
3. `docs/archaeology/phase-0-archaeology-report.md` plus
   `docs/archaeology/phase-0b/reconciliation.md` — what is settled, what is not,
   and four Phase 0 statements the source contradicts.
4. `docs/provenance/ledger.md` — before touching anything derived from an outside
   source.

## Evidence rules

- The original assembly listing is primary evidence. Ports are secondary and
  reference-only until their licence is established in the ledger.
- Never invent a fixture value or a hash. Every fixture carries a source location
  and an extraction method; regenerate with `make fixtures`.
- Label new behavioural claims the way the specification does. "Inferred" is an
  acceptable answer; a confident wrong answer is not.
- `make verify` must pass. A fixture hash change needs a recorded reason in
  `reconciliation.md` before the manifest is regenerated — never regenerate a
  baseline to make a diff pass.
- No ROM image, no emulator capture and no third-party port source enters the
  repository.

## Code rules

- `src/core` links nothing: no SDL, no platform headers. Dependencies point
  inward. See `docs/architecture/module-boundaries.md`.
- Input reaches the simulation as timestamped keystrokes or parsed commands, never
  by a side door.
- Preserve original quirks. Do not "fix" a mechanic because it looks primitive;
  classify it in the quirks ledger and decide deliberately.
- Keep changes small and independently testable, with a regression test for each
  discovered historical behaviour.
- Document deviations from the original explicitly, in
  `docs/specification/clock-and-scheduler.md` §13, rather than smoothing them over.
- Original Mode takes no player-supplied seed and generates no new maps. Those
  belong to a separate future mode.

## Scope discipline

Work the phase that is open. The current phase and its prompt are named in
`README.md`. Do not expand into combat, creatures or mobile UI ahead of the
sequence in the charter.
