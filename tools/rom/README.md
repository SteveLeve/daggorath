# ROM conformance harness

Implements the procedure in
`docs/archaeology/phase-0b/traces/README.md` §3. First run on the ROM
2026-09-25 with MAME 0.264 `coco2b`; firmware and rights are in
`docs/provenance/ledger.md` §1.

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
| `run-capture.sh` | replay `t1`–`t5` through `capture.lua`. Exits 2, and writes no trace, when `DOD_FIRMWARE` lacks Color BASIC 1.3 (`bas13.rom`) and Extended Color BASIC 1.1 (`extbas11.rom`) with the MAME 0.264 `coco2b` SHA-1s |

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
boot a CoCo. The key masks match MAME 0.264 `coco_keyboard` in `coco12.cpp`:
`:rowN` is the PIA0 port A row bit and the mask is the port B column strobe.

`capture.lua` reads `DOD_SYMBOLS`, `DOD_WATCHES`, `DOD_SCRIPT`, `DOD_TRACE`, and
`DOD_RAW`. It does not use MAME's frame notifier (under `-video none` in 0.264
it stopped after 9 frames) or MAME's ioport system (`set_value` lands at the
next frame update). Instead:

- a write tap on `JIFFY`, filtered to `CLOCK`, marks each game interrupt;
- the harness holds SPACE to leave the autoplay demo, lets the level-0 build
  run, and counts jiffy 0 from the first interrupt after `GAME50` is fetched;
- keys are injected at the PIA: a write tap on `$FF02` records the column
  strobe and a read tap on `$FF00` pulls the pressed keys' row bits low.

`DOD_JIFFIES`, when set, stops MAME after that many interrupts past scheduler
entry. Raw samples stay in the raw file, keyed by interrupt. The trace is the
interpretation and reports only state visible in RAM. The alignment and its
limits are written up in `docs/archaeology/phase-1/reconciliation.md` §1.
