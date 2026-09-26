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
| Fight traces | `traces/fight-to-kill` (empty hand, slot 16, type 0, killed at jiffy 234) and `traces/fight-to-death` (slot 8, type 1, death at jiffy 1759). Both are Original Mode on level 0. The kill script was produced by re-simulating after each command. ctest regenerates both traces and compares them byte for byte. A connecting swing adds `SOUND A$KLK2` and `DIALOGUE !!!`; the kill adds `SOUND A$EXP0`. Those lines are `PATTK.ASM` `PATT24` and `PATT40`, recorded in the combat specification. |

## Deviations

| Id | What | Until |
|---|---|---|
| D-8 | Killing creature type 10 or 11 emits `DEFER endgame <type>` and does not run `ENDGAM` or the winner ring. Type 11 still sets the freeze flag. | Phase 5 |

## Not in this phase

`USE`, `INCANT`, `REVEAL`, torch burn-down, `CLIMB`, save, rendering. Hands and `PTORCH` start empty, as `ONCE` leaves `PLHAND` / `PRHAND` unset. Tests write those slots directly because `GET` is Phase 4.

## Capture backlog

Hit/miss sequences, faint timing, and the death screen remain not captured (C-12 and the death-screen row). No claim here is ROM-observed.

## Correction 2026-09-26: HSLOW recovery rounds up

**[SRC]** `COMPLR.ASM:71-82 HSLOW` computes `D = 0 - PDAM`, shifts it with `ASRD6` (`PATTK.ASM:177`, `ASRA` / `RORB`, an
**arithmetic** shift), adds `PDAM`, and stores the result unless it is not positive. An arithmetic
shift rounds toward minus infinity, so each run removes `ceil(PDAM/64)`, at least 1. Damage heals all
the way to 0. The core had implemented `PDAM - (PDAM >> 6)` (floor), which strands damage at 63 for
ever; the specification text ("removes `damage/64`") and the D-12 `FUDGE rest` value of 63 described
that stall as if it were source behaviour. It is not. Symptoms: a player who had taken any bite could
never heal below 63 damage of 160 power, so a fight against a 35-damage viper was lost in about three
bites.

`Game::task_hslow` now follows the listing (16-bit, signed `BGT`). Baselines that change, and why:

| Baseline | Change | Reason |
|---|---|---|
| `traces/fight-to-kill.trace` | `EXERT` damage 1 lower after each `HSLOW` run; a few `HSLOW` reschedule lines shift | small damage now heals by 1 per run instead of not at all |
| `phase-0b/traces/t1-move-turn-look`, `t2-forward-corridor`, `t4-parser-edges` | `EXERT` damage 1 lower after an `HSLOW` run and the `QUEUE`/`TASK` reschedule jiffies that follow the heart rate | same cause: exertion damage under 64 now heals, so the heart rate and `HSLOW` cadence differ. No movement, parse or view line changes |
| `traces/fight-to-death.trace` | the player no longer dies at jiffy 1759: the same script now heals between bites and kills slot 8 (`KILL`) | the death was produced by the stalled floor. The name is historical; death on the hit jiffy stays covered by `test_death_on_exact_jiffy` |

Tests updated for the same reason: `test_hslow_recovery` (was "floor": now heals to 0), the wooden
sword's two damage (checked on the `EXERT` event, since `HSLOW` may already have healed 1), and the
`FUDGE incoming` scaling check (compared on the first hit, where prior damage is 0). New:
`test_damage_recovers_to_zero` in `combat_regressions.cpp`.

`docs/archaeology/phase-5/traces/power-on-to-winner.script` is re-authored, not patched: the planner
(`src/app/dplan.cpp`) was tuned against the stalled recovery. Its `recover()` non-fudge branch called
itself (a no-op), so it made no clock progress; it now idles 20 jiffies. KillImage now tries the
path step both ways and idles when neither works, and with `--fudge` on level 4 or deeper (and while
fainted) the planner rests when damage is above 63. The new script wins at jiffy 190875; sha256 is in
`docs/archaeology/phase-5/reconciliation.md`. `tools/verify_manifest.py` also now skips
subdirectories when listing unlisted files (`phase-6/fixtures/text` has its own manifest, verified
by the third `make verify` line); that was a standing `make verify` failure, not a fixture change.
