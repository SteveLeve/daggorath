# Phase 4 reconciliation

Object commands follow `PGET.ASM`, `PEXAM.ASM`, `PREVEA.ASM`, `PUSE.ASM`, `PINCAN.ASM`, `PCLIMB.ASM`, and `BURNER` in `COMPLR.ASM`.

Labels: **[SRC]** read from the pinned listing; **[INF]** inferred; **[OPEN]** unresolved. Nothing here is ROM-observed.

## Settled

| Item | Label | Disposition |
|---|---|---|
| Bare `CLIMB` | [SRC] | Rejected. The source branches to the command error on a null direction token. Only `CLIMB UP` on a ladder-up and `CLIMB DOWN` on a hole-down or ladder-down change level. |
| `VFTTAB` mapping | [SRC] | The indexes are the ones Phase 1 recorded. Level 0's first group is "down". Feature 3 is ladder down, 2 is hole down, 1 is ladder up, and 0 is a hole arrival that cannot be climbed. |
| `ODBTAB` extent | [SRC] | `OCBFIL` indexes `ODBTAB`, which is `OBJXXX` then `SPCXXX` (`DTABAS.ASM:278`). The seven `SPCXXX` rows (types 18–24) are in `kObjects`. Before that, every successful `INCANT` read past the end of an 18-row table. `ocbfil` now aborts on an out-of-range type, including in release builds. |
| Adjective parsing | [SRC] | `PAROBJ` re-classifies the rejected generic token against `ADJTAB` with `PARSE0`. The core used to consume that token first, so `GET RIGHT WOODEN SWORD` failed. That is fixed and tested. |
| `NEWLVL` and `SYSTCB` | [SRC] | Phase 2 left "enter_level does not call SYSTCB" unresolved. `NEWLVL.ASM` calls `SYSTCB`, which clears all queues and TCBs and sets `RSTART`. The core now does the same, so after `CLIMB` the system tasks run again on that jiffy and `LUKNEW` follows the new `CMOVE` tasks on `Q.TEN`. |
| Bag across `NEWLVL` | [SRC] | The core cleared every `P.OCPTR` before attaching objects, which unlinked the player's bag on `CLIMB`. `NLVL40` writes only creature-owned objects on the new level. Fixed and tested. |
| `DEATH` in a drain | [SRC] | A `FIRE` ring swing costs 78 exertion, enough to pass power without fainting first. `ATTACK LEFT` then `MOVE` in one burst dies on the attack, and the buffered `MOVE` never runs (`PATTK` has no `SYNC`, and `DEATH` is `BRA *`). |

## Tests

`tests/conformance/object_regressions.cpp` covers the issue's list:

- the torch minute boundary, the light clamp, and the timer threshold
- `REVEAL` below and above `reveal * 25`, and `EXAMINE` with and without a creature
- each flask: `THEWS`, `HALE`, `ABYE`
- each scroll, revealed and unrevealed: `VISION`, `SEER`
- each ring word, held and not held: `FINAL`, `ENERGY`, `ICE`, `FIRE`
- `CLIMB` at features 0 to 3 and on a cell with none, plus bare `CLIMB`
- burden through `GET` and `DROP` weights and `PMOV90`
- the starting inventory against `initial-state.json`
- every `CMDTAB` verb in `tokens.json` reaching a handler, except `ZSAVE` and `ZLOAD`

## Still deferred

| Id | What | Until |
|---|---|---|
| D-8 | Wizard kill still emits `DEFER endgame` | Phase 5 |
| D-9 | The final-ring incantation emits `DEFER winner` instead of `WINNER` | Phase 5 |

`ZSAVE` and `ZLOAD` still report `UNIMPLEMENTED`.

## Open

| Item | Label |
|---|---|
| Durations of `A$TORC`, `A$RING`, and `A$FLAS` | [OPEN] D-4. They cost no simulated time here. |