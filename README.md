# Dungeons of Daggorath — preservation project

A faithful modern implementation of the 1983 TRS-80 Color Computer game, built
from the original assembly listing as the specification. Original Mode is a
historical reproduction: the same five fixed maps, the same 60 Hz timing, the same
command language, the same quirks. Optional enhanced modes may come later, attached
at extension points, never by editing Original Mode.

The simulation core is headless, deterministic and links nothing. Android and iOS
are the eventual targets; neither is being built yet.

## State

| | |
|---|---|
| **Phase 0** | complete — [source archaeology report](docs/archaeology/phase-0-archaeology-report.md) |
| **Phase 0b** | complete — [executable evidence pack](docs/archaeology/phase-0b/README.md): 15 fixtures, jiffy-by-jiffy scheduler specification, headless C++20 slice, 68 conformance checks passing |
| **Phase 1** | cartridge bytes match catalog 26-3093; ROM captures are in [reconciliation](docs/archaeology/phase-1/reconciliation.md) §1. Next: [apply the owner's decision on #13](docs/prompts/phase-1-apply-issue-13.md) |

The pinned listing at `a94326f`, assembled with LWTOOLS 4.25, is byte-identical
to the Tandy catalog 26-3093 cartridge image. MAME 0.264 `coco2b` has run that
image. Which claims are ROM-observed, and which are still source-derived, is
§1 of the Phase 1 reconciliation. The cartridge file is not in the repository.

## What runs today

The core implements the 24-bit RNG, maze generation for all five levels, the
discrete clock and scheduler, the unchecked 32-byte keyboard buffer, the line
editor, the original parser, and `MOVE`, `TURN` and `LOOK` with the movement
exertion path and heart-rate update. Entering a level births creatures from
the `CMXLND` counts and hangs creature-owned objects on them. `CREGEN`
increments the current level's matrix every five minutes (and once on the
opening lap) and does not create a creature until the next entry.
Every other command reports `UNIMPLEMENTED` rather than approximating.
Creature movement, combat, magic, rendering, audio and save/load are absent
by design.

`dcli` runs timestamped keystroke scripts and emits diff-friendly traces, so the
same script can later be replayed against a ROM capture and compared line by line.

## Build

Requires CMake 3.20+, a C++20 compiler and Python 3.9+.

```sh
make sources     # fetch the pinned assembly listing into third_party/ (evidence, never committed)
make fixtures    # re-extract every fixture, regenerate the lexicon header
make build       # configure and build
make test        # run the conformance tests
make traces      # regenerate every trace from its script
make verify      # check fixture hashes against MANIFEST.json
make all         # all of the above
```

Fixtures and the generated lexicon header are committed, so `make build && make
test` works without fetching the listing. Fetch it when you need to re-derive
something or extend the extractor.

Without CMake, the slice is five translation units:

```sh
g++ -std=c++20 -Wall -Wextra -O2 -Isrc/core/include \
    -DDAG_FIXTURE_DIR='"'$PWD'/docs/archaeology/phase-0b/fixtures"' \
    src/core/*.cpp tests/conformance/conformance_tests.cpp -o /tmp/conformance_tests
/tmp/conformance_tests

g++ -std=c++20 -O2 -Isrc/core/include src/core/*.cpp src/app/dcli.cpp -o /tmp/dcli
/tmp/dcli --maze-summary
/tmp/dcli --script docs/archaeology/phase-0b/traces/t2-forward-corridor.script --jiffies 200
```

## Layout

```text
src/core/           simulation: clock, scheduler, RNG, world, commands. Links nothing.
src/presentation/   placeholder: visibility, display mode, ordered screen/audio events
src/input/          placeholder: touch, keyboard and controller adapters -> commands
src/platform/       placeholder: SDL3, storage, Android and iOS packaging
src/app/            dcli trace harness today; desktop application later
tests/conformance/  cross-checks against the extracted fixtures
tools/              fixture extractor, lexicon generator, manifest verifier
tools/rom/          ROM conformance harness (scaffolding; no ROM ever committed)
docs/               charter, specification, architecture, provenance, licensing, prompts, archaeology
```

Dependencies point inward: outer modules depend on `core`, never the reverse. See
[`docs/architecture/module-boundaries.md`](docs/architecture/module-boundaries.md).

## How this project decides things

The listing is primary evidence; later ports are secondary and reference-only
until their licence is established. Every behavioural rule in the specification is
labelled source-proven, ROM-observed, inferred, or unresolved, and "unresolved" is
a normal outcome. Fixtures carry a source location and an extraction method, and
their hashes are verified rather than trusted. Original quirks are classified and
preserved deliberately, not tidied away — the unchecked keyboard buffer, the line
that dispatches itself when full, and the 256 RNG draws when `SECOND` is zero are
all behaviour, not bugs to fix.

Four statements in the Phase 0 report turned out to be contradicted by the source;
they are corrected in
[`docs/archaeology/phase-0b/reconciliation.md`](docs/archaeology/phase-0b/reconciliation.md)
and summarised in the report's own reconciliation section. Expect more of that, and
prefer a recorded correction to a quiet edit.

Working rules for agents and contributors are in [`CLAUDE.md`](CLAUDE.md).

## Licensing

Unsettled, and treated as an engineering requirement rather than a footnote. The
Morgan preservation grant covers reproduction of the game on condition that its
original form is preserved, but it does not resolve the manual, the artwork, later
port copyrights, trademark use or commercial terms. The extracted lexicon and
tuning tables are copied original data, not computed behaviour, and the generated
`src/core/include/daggorath/lexicon_tables.hpp` carries that data into the build.

No ROM image, emulator capture, manual scan or third-party port source belongs in
this repository. See [`docs/licensing/README.md`](docs/licensing/README.md) and
[`docs/provenance/ledger.md`](docs/provenance/ledger.md). Qualified legal review
before any public distribution.
