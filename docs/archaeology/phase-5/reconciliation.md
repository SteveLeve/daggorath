# Phase 5 reconciliation

`ENDGAM` and `WINNER` replace the `DEFER` stubs from Phases 3 and 4. `ZSAVE` and `ZLOAD` no longer report `UNIMPLEMENTED`.

## Remaining

| Item | Owner |
|---|---|
| Cassette leader bytes, video buffers, and raw queue pointers inside the `$0200`–`MM.END` image | not modeled; ADR-0005 records the modeled subset |
| A power-on to `WINNER` playthrough script | not yet authored; endings are reachable from prepared states |
| Drawing the death and winner screens | Phase 6 |
| SDL presentation | Phase 7 |

No other verb reports `UNIMPLEMENTED`.
