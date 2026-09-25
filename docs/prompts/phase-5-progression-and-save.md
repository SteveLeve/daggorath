Carry out **Phase 5: progression, endings and save/load** — the phase that
completes the headless game. Read the charter, `CLAUDE.md`, the roadmap,
ADR-0005, ADR-0008 and the Phase 4 reconciliation first. Produce working
artifacts, not a plan.

**Preservation requirement.** The wizard-image kill runs `ENDGAM` as the source
does (dialogue, equipment stripping, imposed weight, regeneration of level 3,
random relocation); the true-wizard kill freezes creatures and sets up the final
ring; the final ring's full incantation reaches `WINNER`; `DEATH` presents as
the source does. `ZSAVE`/`ZLOAD` save and restore exactly what `PZTAPE` and
`COMMON` save and restore — including what they lose.

Work in this order.

1. **Specify** progression in a new `docs/specification/progression-and-save.md`.
   For save: list the exact RAM blocks written and read, and what a load
   re-derives. Settle ADR-0005 with a dated Resolution.
2. **Retire** every `DEFER` stub left by Phases 2–4 and their deviations.
3. **Implement** endings, `ZSAVE`/`ZLOAD` in Original Mode semantics, and the
   total-state serialiser (for tests and the future suspend snapshot; the
   snapshot is not exposed as a game command).
4. **Tests:** both endings from a prepared state; death; save then load then
   compare to the source's restored subset; total-state round trip is
   bit-identical and a replay from a snapshot matches continuous play.
5. **Playthrough:** commit a `dcli` script and trace that plays from power-on to
   `WINNER`. It may be long; it must be deterministic. Record how it was
   authored (by hand, or with a search tool that lives under `tools/`).
6. **Reconcile** Phase 5; update `README.md` "What runs today"; mark the headless
   core complete in the roadmap.

**Do not build:** rendering, audio output, SDL, UI, enhanced-mode options.

**Completion gate.** `make all` output; no verb reports `UNIMPLEMENTED`; the
playthrough trace reproduces byte for byte on two runs; deviation table listing
only those that genuinely remain, each with its owner (Track R or a named
phase).
