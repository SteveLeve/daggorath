# Phase 3 reconciliation

Combat follows `PATTK.ASM` and the attack branch of `CRETUR.ASM`. The independent fixture generator is `tools/gen_combat_fixtures.py`. The C++ `scal16` / `attack_hits` / `apply_damage` path is checked against that file.

## Closed

| Item | Disposition |
|---|---|
| D-7, `CMOVE` stopped before `JSR ATTACK` | Retired. A same-cell action resolves `ATTACK` and `DAMAGE`. |
| Phase 0 "VERIFY byte wrapping and signed flags" | Closed for `ATTACK` and `SCAL16`. See `docs/specification/combat-and-items.md`. Creature death is unsigned `power > damage`. Player death is unsigned `power < damage`. |

| `PATTK` and `SYNC` | **[SRC]** `PATTK.ASM` has no `DEC UPDATE` / `SYNC`. It ends in `SWI HUPDAT` and falls through `ASRD7` to `RTS`. `cmd_attack` therefore does not set `sync_pending_`, and a buffered command after `ATTACK` runs in the same `PLAYER` drain. |
| `DEATH` inside a drain | **[SRC]** `HUPDAT.ASM` `DEATH` ends in `BRA *`. `task_player` stops draining once the scheduler is halted. With only generic object stats in this phase, one command's own exertion cannot pass power without first fainting, so the buffered-command case is tested once `REVEAL` exists (Phase 4). |
| Class constants | **[SRC]** `K.RING` = 1 and `K.SHIE` = 3 (`CD.ASM`). `EMPHND` is class 4, magic offense 0, and physical offense 5 (`COMDAT.ASM` `INI P.OCCLS+EMPHND`). |
| RNG draws per swing | **[SRC]** A ring swing draws no byte. Any other swing draws one byte in `ATTACK`, plus a second byte in `PATT22` when it connects without a live torch. A swing with no creature on the cell draws nothing. `tests/conformance/combat_regressions.cpp` checks the ring case against a control run. |
| Fixture meaning | The Python generator and the C++ core are independent implementations of the same listing reading. Agreement on the 88 `S`, 240 `D`, and 256 `A` rows shows internal consistency, not agreement with the ROM. |
| Prompt §4 regression tests | `tests/conformance/combat_regressions.cpp`: attack while fainted, the darkness gate against a lit control, empty hand, ring bypass, same-jiffy player and creature attacks, kill to `CMXLND` to re-entry, and death on the jiffy of the hit. |
| Fight traces | `traces/fight-to-kill` (empty hand, slot 16, type 0, killed at jiffy 234) and `traces/fight-to-death` (slot 8, type 1, death at jiffy 1759). Both are Original Mode on level 0. The kill script was produced by re-simulating after each command. ctest regenerates both traces and compares them byte for byte. |

## Deviations

| Id | What | Until |
|---|---|---|
| D-8 | Killing creature type 10 or 11 emits `DEFER endgame <type>` and does not run `ENDGAM` or the winner ring. Type 11 still sets the freeze flag. | Phase 5 |

## Not in this phase

`USE`, `INCANT`, `REVEAL`, torch burn-down, `CLIMB`, save, rendering. Hands and `PTORCH` start empty, as `ONCE` leaves `PLHAND` / `PRHAND` unset. Tests write those slots directly because `GET` is Phase 4.

## Capture backlog

Hit/miss sequences, faint timing, and the death screen remain not captured (C-12 and the death-screen row). No claim here is ROM-observed.
