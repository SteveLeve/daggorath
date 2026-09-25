# ROM conformance harness (scaffolding)

Empty by design. This directory is where the emulator harness lands, and nothing
here should assume a ROM is present.

**Nothing in this repository ships a ROM image, and `.gitignore` refuses `*.rom`,
`*.ccc` and `captures/`.** Rights to hold a retail image are the developer's own
concern and must be recorded in `docs/provenance/ledger.md` before any capture is
used as evidence.

Planned contents, in the order Phase 1 needs them:

| File | Purpose |
|---|---|
| `assemble.sh` | build the pinned listing with `lwasm`, hash the cartridge, diff it against a retail image, and write the result to `docs/provenance/rom-diff.md` |
| `watchlist.tsv` | symbols to sample every interrupt, with widths — already written, see below |
| `capture.lua` | emulator script: sample the watchlist at 60 Hz, inject keystrokes from a trace script, emit the trace format used in `docs/archaeology/phase-0b/traces/` |
| `trace_diff.py` | diff a ROM trace against a `dcli` trace and report the first divergence with its jiffy |

The capture procedure itself — prerequisites, priority order, what to record — is
specified in `docs/archaeology/phase-0b/traces/README.md` §3. Implement to that
document rather than inventing a second procedure.
