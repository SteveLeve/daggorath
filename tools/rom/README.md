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
entry. `DOD_SECONDS` is the `-seconds_to_run` backstop (default 120).
Optional arguments are extra keystroke scripts; the default is `t1`–`t5`.

The trace reports RAM changes and, once `SCHED` is running, `TASK` and `LINE`.
Separate gitignored logs sit next to the raw file: `.task.tsv` (dispatch),
`.spin.tsv` (`DGEN90` entry and exit), `.sound.tsv` (`SNOISE` and `THUD`),
`.pop.tsv` (`NEWLVL`, `GAME50`, `CREGEN`). `DOD_POKE_SECOND` together with
`DOD_POKE_ON_SPIN` writes `SECOND` at the `LDB` before that spin. Those
captures are harness-modified and the spin log says so.

```sh
python3 tools/rom/trace_diff.py trace-a trace-b
python3 tools/rom/trace_diff.py --relative-to-init --kinds LINE,TURN,MOVE trace-a trace-b
```

The relative comparison is printed after the literal first divergence. It
skips `INIT` and drops the clock column. The default two-argument invocation
is unchanged.
