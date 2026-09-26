# Phase 3 reconciliation

Combat follows `PATTK.ASM` and the attack branch of `CRETUR.ASM`. The independent fixture generator is `tools/gen_combat_fixtures.py`. The C++ `scal16` / `attack_hits` / `apply_damage` path is checked against that file.

## Closed

| Item | Disposition |
|---|---|
| D-7, `CMOVE` stopped before `JSR ATTACK` | Retired. A same-cell action resolves `ATTACK` and `DAMAGE`. |
| Phase 0 "VERIFY byte wrapping and signed flags" | Closed for `ATTACK` and `SCAL16`. See `docs/specification/combat-and-items.md`. Creature death is unsigned `power > damage`. Player death is unsigned `power < damage`. |

## Deviations

| Id | What | Until |
|---|---|---|
| D-8 | Killing creature type 10 or 11 emits `DEFER endgame <type>` and does not run `ENDGAM` or the winner ring. Type 11 still sets the freeze flag. | Phase 5 |

## Not in this phase

`USE`, `INCANT`, `REVEAL`, torch burn-down, `CLIMB`, save, rendering. Hands and `PTORCH` start empty, as `ONCE` leaves `PLHAND` / `PRHAND` unset. Tests write those slots directly because `GET` is Phase 4.

## Capture backlog

Hit/miss sequences, faint timing, and the death screen remain not captured (C-12 and the death-screen row). No claim here is ROM-observed.
