Carry out a **Track R session: ROM observation** for the Dungeons of Daggorath
preservation project. Read `docs/adr/0003-rom-observation-track.md`,
`docs/provenance/rom-diff.md`, `docs/provenance/ledger.md`,
`docs/archaeology/phase-0b/traces/README.md` §3,
`docs/planning/capture-backlog.md` and `CLAUDE.md` first.

**Precondition — stop if unmet.** A legal source for the CoCo system ROMs
(Color BASIC 1.2, Extended Color BASIC 1.1, or a documented replacement firmware)
is recorded in the ledger with its rights basis. Without it, do nothing except
record the attempt in `rom-diff.md`. Never download ROMs from an unrecorded
source; never commit a ROM or a capture.

**Preservation requirement.** Observations are recorded raw, separately from
interpretation. A capture that disagrees with the core is a finding, not a bug
in the capture, until shown otherwise.

Work in this order.

1. Record emulator and firmware in the ledger (version, hash, rights basis).
   If replacement firmware is used, first show that the cartridge boots and that
   a Phase 0b script produces a sensible trace, and record that validation.
2. Run `tools/rom/capture.lua`'s frame loop for the backlog items in priority
   order, starting with the five Phase 0b scripts.
3. For each capture run `tools/rom/trace_diff.py` against the `dcli` trace and
   record the first divergence, or none, in the current phase's reconciliation
   (or `docs/archaeology/track-r/reconciliation.md` if no phase is open).
4. Promote labels to ROM-observed only for rules a capture actually exercised;
   retire or re-label deviations (D-1, D-2, D-4) with the measurements.
5. A divergence becomes its own issue and its own change: specification first,
   then core, with a regression test.

**Completion gate.** Table of captures with first divergence each; list of
labels promoted; deviations changed; no ROM or capture staged
(`git status --porcelain` output).
