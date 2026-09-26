# Phase 5 reconciliation

`ENDGAM` and `WINNER` replace the `DEFER` stubs from Phases 3 and 4. `ZSAVE` and `ZLOAD` no longer report `UNIMPLEMENTED`.

## Remaining

| Item | Owner |
|---|---|
| Cassette leader bytes, video buffers, and raw queue pointers inside the `$0200`–`MM.END` image | not modeled; ADR-0005 records the modeled subset |
| A power-on to `WINNER` playthrough script | not yet authored. A prepared hold of the level-4 supreme ring plus `INCANT FINAL` does reach `WINNER`. The ring is carried by creature type 11 (power 8000, physical defense 0, magic defense 6). At power 160 a wooden sword deals 0, an elvish sword deals 3, and the fire ring deals 14. The fire ring's exertion is large enough that the player cannot swing it repeatedly at that power. `HSLOW` recovers one sixty-fourth of damage per heartbeat and stops reducing once damage is below 64, so rest bottoms out at 63. |
| Drawing the death and winner screens | Phase 6 |
| SDL presentation | Phase 7 |

No other verb reports `UNIMPLEMENTED`.
