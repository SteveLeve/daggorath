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
| `assemble.sh` | build the pinned listing with `lwasm`, write `build/rom/symbols.tsv` from the listing's label column, hash the cartridge, and print a byte diff against `DOD_ROM` when that variable names an image. Exits 2 when `lwasm` is missing and does not rewrite `rom-diff.md`. Exits 1 when a watchlist symbol is missing |
| `watchlist.tsv` | symbols to sample every interrupt, with widths |
| `capture.lua` | MAME script: sample the watchlist at 60 Hz, inject `<jiffy> <KEY>` keystrokes on the CoCo matrix, write a raw sample file and a separate interpreted trace |
| `trace_diff.py` | diff two traces and report the first divergence with its jiffy and both lines |
| `run-capture.sh` | replay `t1`–`t5` through `capture.lua`. Exits 2, and writes no trace, when `DOD_FIRMWARE` lacks Color BASIC 1.2 and Extended Color BASIC 1.1 with the MAME 0.264 `b12e11` SHA-1s |

```sh
python3 tools/rom/trace_diff.py trace-a trace-b
DOD_ROM=/path/to/image.rom tools/rom/assemble.sh
```

`capture.lua` can check its file parsers without a machine:

```sh
DOD_SELFTEST=1 DOD_SYMBOLS=symbols.tsv DOD_WATCHES=tools/rom/watchlist.tsv \
  DOD_SCRIPT=path/to/script lua tools/rom/capture.lua
```

That run checks the watchlist, the symbol table, and the key table. It does not
boot a CoCo. The key masks match MAME 0.264 `coco_keyboard` in `coco12.cpp`.
The `:rowN` tags are the root-device subtags from MAME 0.264 `port_alloc`. They have not been read back from a running machine. A frame
capture still needs firmware, recorded in `docs/provenance/rom-diff.md`.

`capture.lua` reads `DOD_SYMBOLS`, `DOD_WATCHES`, `DOD_SCRIPT`, `DOD_TRACE`, and
`DOD_RAW`. `DOD_JIFFIES`, when set, stops MAME after that many sampled frames.
Raw samples stay in the raw file. The trace is the interpretation. On MAME
0.264 the frame notifier runs at the end of the frame, so keys for jiffy 0 are
pressed from the reset notifier and later keys are pressed after the previous
frame is sampled.
