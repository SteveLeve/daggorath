# ROM conformance harness

Implements the procedure in
`docs/archaeology/phase-0b/traces/README.md` §3. The Phase 1 run did not execute
it; the reason is only in `docs/provenance/rom-diff.md`.

**Nothing in this repository ships a ROM image, and `.gitignore` refuses `*.rom`,
`*.ccc` and `captures/`.** A retail image is the developer's to hold, and its
hash and rights basis go in `docs/provenance/ledger.md` before any capture is
used as evidence.

| File | Purpose |
|---|---|
| `assemble.sh` | build the pinned listing with `lwasm`, hash the cartridge, and print a byte diff against `DOD_ROM` when that variable names an image. Exits 2 when `lwasm` is missing and does not rewrite `rom-diff.md` |
| `watchlist.tsv` | symbols to sample every interrupt, with widths |
| `capture.lua` | MAME script: sample the watchlist at 60 Hz, inject `<jiffy> <KEY>` keystrokes on the CoCo matrix, write a raw sample file and a separate interpreted trace |
| `trace_diff.py` | diff two traces and report the first divergence with its jiffy and both lines |

```sh
python3 tools/rom/trace_diff.py trace-a trace-b
DOD_ROM=/path/to/image.rom tools/rom/assemble.sh
```

`capture.lua` reads `DOD_SYMBOLS`, `DOD_WATCHES`, `DOD_SCRIPT`, `DOD_TRACE`, and
`DOD_RAW`. Raw samples stay in the raw file. The trace is the interpretation.
