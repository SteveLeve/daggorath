# Original game data notice

This repository is an unofficial, non-commercial reconstruction of *Dungeons of
Daggorath*. It is not affiliated with or endorsed by Tandy Corporation,
Radio Shack, DynaMicro or Unified Technologies.

[`LICENSE`](LICENSE) (MIT) covers only the project's own work. The artifacts
below carry **original game content** copied or decoded from the assembly
listing. They are **not** MIT-licensed, and this project grants no licence over
them. They are included in reliance on Douglas J. Morgan's public preservation
grant. That grant is conditioned on preserving the game's original, unaltered
form. Its scope is still open (see
[`docs/licensing/README.md`](docs/licensing/README.md)). The provenance for
each artifact is in [`docs/provenance/ledger.md`](docs/provenance/ledger.md) §4.

| Artifact | Original content (sources: the header's own "Ultimate source" line, or ledger §4) |
|---|---|
| `src/core/include/daggorath/lexicon_tables.hpp` (generated) | player-facing lexicon |
| `src/core/include/daggorath/sound_tables.hpp` (generated) | sound parameters |
| `src/presentation/include/daggorath/vector_tables.hpp` (generated) | vector art and font |
| `src/presentation/include/daggorath/text_tables.hpp` (generated) | screen text |
| `src/core/population.cpp`, `src/core/include/daggorath/population.hpp`, `src/core/include/daggorath/maze.hpp` | object, creature and vertical-feature table rows and `LVLTAB` seeds, transcribed by hand |
| fixtures under `docs/archaeology/**/fixtures/` that ledger §4 classes as copied data | tables, lexicon, sounds, vectors, text |

Each generated header opens with a rights notice. The generators in `tools/`
emit it on every regeneration.

Distribution posture until qualified legal review: source repository only. No
binary release, app-store listing or monetisation.
