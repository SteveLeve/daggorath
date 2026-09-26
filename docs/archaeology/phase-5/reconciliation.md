# Phase 5 reconciliation

`ENDGAM` and `WINNER` replace the `DEFER` stubs from Phases 3 and 4. `ZSAVE` and `ZLOAD` no longer report `UNIMPLEMENTED`.

## Remaining

| Item | Owner |
|---|---|
| Cassette leader bytes, video buffers, and raw queue pointers inside the `$0200`–`MM.END` image | not modeled; ADR-0005 records the modeled subset |
| A power-on to `WINNER` playthrough script | not yet authored. On a frozen level 4, holding the vulcan ring, `INCANT FIRE`, walking to the type-11 creature, and resting to damage 63 between swings kills it in 572 hits of 14. `ENDGAM` clears the hands. `GET RIGHT RING` takes the dropped supreme ring (spec[1] = 18) and `INCANT FINAL` emits `WINNER`. A wooden sword deals 0 and an elvish sword deals 3 at power 160, so neither finishes that creature. `HSLOW` stops reducing damage once it is below 64, so rest bottoms out at 63. The same fight with live creatures, from power-on on level 0, is still unscripted. |
| Drawing the death and winner screens | Phase 6 |
| SDL presentation | Phase 7 |

No other verb reports `UNIMPLEMENTED`.
