# Phase 0b — executable evidence pack

Produced 2026-09-24 against the reconstructed 1983 assembly listing at commit
`a94326f00ebb16a106b540c58bc2ccf5f7b66dac`. No mobile UI, no combat.

**Read this first:** no retail ROM and no emulator were available in the
environment this phase ran in, so **nothing in this pack is ROM-verified**. Every
conclusion is source-derived from a listing that carries 2022 `lwasm` edits. See
[`docs/provenance/ledger.md`](../../provenance/ledger.md) §5 and
[`traces/README.md`](traces/README.md) §1.

## Contents

| Path | What it is |
|---|---|
| [`../../provenance/ledger.md`](../../provenance/ledger.md) | **moved out of this pack, now a living document:** pinned commit, per-file hashes, tool versions, rights position, what is copied data versus computed |
| [`../../specification/clock-and-scheduler.md`](../../specification/clock-and-scheduler.md) | **moved out of this pack, now a living document:** jiffy-by-jiffy clock, queue and dispatch specification, every rule labelled source-proven / inferred / unresolved |
| `reconciliation.md` | each investigated Phase 0 `VERIFY` item: evidence, result, confidence; corrections to Phase 0; what is still open |
| `fixtures/` | machine-readable fixtures plus `MANIFEST.json` with a hash per file |
| `traces/` | timestamped keystroke scripts, their outputs, and the ROM capture procedure to run later |
| [`../../../tools/extract_fixtures.py`](../../../tools/extract_fixtures.py) | the extractor: reads the listing, emits every fixture, computes every hash |
| [`../../../tools/gen_lexicon_header.py`](../../../tools/gen_lexicon_header.py) | generates the C++ lexicon header from `fixtures/tokens.json` |
| [`../../../src/core/`](../../../src/core/) | the reference slice, promoted into the project layout: `src/core`, `src/app/dcli.cpp`, `tests/conformance/` |

## Fixtures

| File | Contents |
|---|---|
| `rng.json`, `rng-vectors.txt` | `RANDOX` reference vectors: three seeds × 16 outputs + final seed state |
| `mazes.json` | per-level `LVLTAB` window, maze SHA-256, cleared-cell count, RNG state before/after the `DGEN90` spin, and the entry-time invariance matrix |
| `maze-level-0..4.bin` | the canonical 1024-byte maze arrays themselves |
| `tokens.json` | all four token tables decoded from the packed 5-bit strings, with the packed bytes retained |
| `parser-prefixes.json` | shortest unique abbreviation per token under `PARSER`'s rule |
| `creatures.json` | 12 creature definition blocks plus the 5 × 12 initial population matrix and the `CREGEN` rule |
| `objects.json` | generic classes and weights, level objects with counts and reveal requirements, special objects, torch/shield parameters |
| `vertical-features.json` | `VFTTAB` bytes and grouped records (level mapping still open) |
| `clock.json` | rollovers, queue codes, TCB layout, startup task order, reschedule intervals, buffer sizes |
| `initial-state.json` | starting position, power, carried weight, bag, and the evidence for each |

Maze serialization, which the hashes depend on: **1024 bytes, row-major, index
`row*32 + col`, one byte per cell; bit pairs low-to-high North, East, South, West;
`00` passage, `01` regular door, `10` secret door, `11` wall.**

## Build and run

The build instructions live in the [root README](../../../README.md); this pack no
longer carries its own copy. In short, from the repository root:

```sh
make sources     # fetch the pinned listing into third_party/
make fixtures    # re-extract every fixture, regenerate the lexicon header
make build test  # build and run the conformance tests
make traces      # regenerate every trace in this pack from its script
make verify      # check these fixture hashes against MANIFEST.json
```

The code moved during the Phase 1 reorganisation: `reference/` became
`src/core`, `src/app/dcli.cpp` and `tests/conformance/`, and the fixture path
passed to the build is now `-DDAGGORATH_FIXTURE_DIR`. The fixtures, traces and
scripts in this pack are unchanged, and `make verify` confirms that.

### `dcli` options

| Option | Meaning |
|---|---|
| `--script FILE` | timestamped keystroke script, `<jiffy> <KEY>` per line; `KEY` is one character or `SPACE`, `CR`, `BS`; `#` comments |
| `--jiffies N` | how many 1/60 s boundaries to simulate (default 600) |
| `--second S` | the `SECOND` counter at level entry, fed to `DGEN90` (default 1) |
| `--dump-maze FILE` | write the 1024-byte maze array |
| `--trace FILE` | write the trace instead of stdout |
| `--maze-summary` | per-level cleared-cell count and RNG state around the spin |

## What the tests check

68 checks, all passing as of this pack. In summary:

- the RNG reproduces all three fixture vectors draw-for-draw and its seed state
  after 16 calls;
- the level-0 maze is **byte-identical** to `fixtures/maze-level-0.bin`, which was
  produced by a separate Python transliteration — the test reports the first
  divergent byte with its row and column if they ever differ;
- all five levels carve exactly 500 cells and have reciprocal edges;
- level 0 is byte-identical at `SECOND` = 0, 1, 7, 30, 59, while the post-spin RNG
  state differs, and `SECOND = 0` spins 256 times;
- all 500 carved cells are reachable from the start cell under `STEPOK`;
- parser behaviour including ambiguous `Z`, over-long `MOVEX`, empty input, and
  `BACK` versus rejected `BACKWARD`;
- clock rollovers at 6, 60 and 3600 jiffies;
- `TURN LEFT/RIGHT/AROUND`, rejection of `TURN UP`, blocked `MOVE` still paying
  exertion, a full command typed inside one jiffy, and `LOOK`.

The maze check is a genuine cross-implementation comparison: the C++ generator and
the Python extractor were written from the assembly independently, and the test
reads the fixture off disk rather than recomputing it.

## Scope boundary

In the reference slice: the RNG, fixed level-0 (and 1–4) maze generation, the
discrete clock and scheduler, the 32-byte keyboard buffer, the line editor,
`PARSER`, `MOVE`, `TURN`, `LOOK`, the movement exertion path, `HUPDAT`/`HSLOW`.

Deliberately absent: combat, creatures, items, magic, rendering, audio, save/load,
and every other command, which report `UNIMPLEMENTED` rather than approximating.
The core links nothing — no SDL, no platform API — and the library target has no
dependencies at all.

Known deviations from the original are enumerated in
[`../../specification/clock-and-scheduler.md`](../../specification/clock-and-scheduler.md) §13 (D-1 … D-5) rather than hidden.
