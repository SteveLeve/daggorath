# ADR-0006 — Extracted historical data

**Status:** Accepted, 2026-09-25.

## Context

The lexicon and tuning tables already enter the build through the generated
`src/core/include/daggorath/lexicon_tables.hpp`. Later phases need more copied
original content: `ATTACK`/`DAMAGE` factor tables, object tables, vector
geometry (`VARC`, `VERT`, `VOBJ`, `D3`, `D4`), scale tables (`NORSCL`,
`HLFSCL`, `BAKSCL`), the font, and `SOUNDS` waveform parameters. Their rights
are open (`licensing/README.md` question 4).

## Decision

1. Every such table is extracted by `tools/extract_fixtures.py` (or a sibling
   tool) from the pinned listing into a fixture with source location and method,
   and hashed in `MANIFEST.json`. Nothing is typed in by hand.
2. Code consumes it only through generated headers under
   `src/core/include/daggorath/` (core data) or `src/presentation/` (geometry,
   font, sound parameters), each carrying a banner naming its fixture and
   provenance class.
3. Generated data files are listed in `provenance/ledger.md` §4 as the set that
   needs the grant or permission to ship; a build option can exclude them to
   show what remains is new code.
4. No sampled audio or screenshots from ports or emulators enter the tree;
   sound is synthesised from the listing's parameters.

## Consequences

- Distribution review (Phase 9 precondition) has a closed list of copied
  content to evaluate.
