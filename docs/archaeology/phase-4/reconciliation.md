# Phase 4 reconciliation

Object commands follow `PGET.ASM`, `PEXAM.ASM`, `PREVEA.ASM`, `PUSE.ASM`, `PINCAN.ASM`, `PCLIMB.ASM`, and `BURNER` in `COMPLR.ASM`.

## Settled

Bare `CLIMB` is rejected. The source branches to the command error on a null direction token. Only `CLIMB UP` on a ladder-up and `CLIMB DOWN` on a hole-down or ladder-down change level.

`VFTTAB` indexes are the ones Phase 1 recorded. Level 0's first group is "down".

## Still deferred

| Id | What | Until |
|---|---|---|
| D-8 | Wizard kill still emits `DEFER endgame` | Phase 5 |
| D-9 | The final-ring incantation emits `DEFER winner` instead of `WINNER` | Phase 5 |

`ZSAVE` and `ZLOAD` still report `UNIMPLEMENTED`.
