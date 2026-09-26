# Phase 5 reconciliation

Labels: **[SRC]** read from the pinned listing; **[INF]** inferred; **[OPEN]** unresolved. Nothing here is ROM-observed.

`ENDGAM`, the ring riddle, and `WINNER` replace the `DEFER` stubs from Phases 3 and 4 (D-8 and D-9 are retired in `clock-and-scheduler.md` §13). No verb reports `UNIMPLEMENTED`.

## Settled

| Item | Label | Disposition |
|---|---|---|
| Dialogue text | [SRC] | `tools/decode_outsti.py` decodes every `OUTSTI` string. `DEATH`, `ENDGAM`, and `WINNER` emit their exact strings, including the leading carriage return and the trailing `...`. The earlier core printed the death line without its periods, and the endings printed nothing. |
| `ENDGAM` hands | [SRC] | `ENDGAM` only replaces `BAGPTR` with `PTORCH`. The earlier core also cleared both hands. |
| What `ZSAVE` writes | [SRC] | The whole `DP.BEG`–`MM.END` range (ADR-0005). The earlier core stored a 20-field subset and dropped creatures, objects, queues, and part of the clock. `ram_image()` now holds every modeled field, and `ZLOAD` resumes at the exact post-save state. |
| When the tape runs | [SRC] | `SCHED1`, after the `PLAYER` task is requeued. `LOAD90` then clears `ZFLAG`, runs `INIVU`, and ends the lap. |
| `FNDCEL` order | [SRC] | `RNDCEL` draws the column, then the row. |
| Suspend snapshot | — | `snapshot()` round-trips byte for byte, and a replay after restore matches the original run line for line, including into a game whose level, clock, and halt state had diverged. |

## Trace baseline change

`docs/archaeology/phase-3/traces/fight-to-death.trace` gains one line on this branch: `1759 0:0:35.6.0 DIALOGUE ^ YET ANOTHER DOES NOT RETURN...`, the `DEATH` `OUTSTI` string that Phase 5 emits. No other line changes. The trace is regenerated here for that reason only. `fight-to-kill.trace` is unchanged.

## Tests

`tests/conformance/progression_regressions.cpp`: both endings, `WINNER`, the death line, `ZSAVE`/`ZLOAD` resuming at the save point with creatures moving, an absent name (D-11), the RAM image restoring byte for byte into a different game, and the snapshot round trip plus replay.

## Deviations

| Id | Owner |
|---|---|
| D-11 `ZLOAD` of an absent name reports `???` | Permanent, Original Mode cassette substitute (ADR-0005) |
| D-12 `FUDGE incoming` / `FUDGE rest` | **Not source behaviour.** Harness API only (ADR-0007). Default `Game()` stays at 100% incoming damage. `FUDGE incoming 25` multiplies creature-to-player damage by 1/4. `FUDGE rest` writes `PDAM = 63` (the HSLOW floor) instead of simulating hours of recovery. Player hits are not scaled. Checkpoints under `.cache/playthrough/` restore `snapshot()` and are not a game command. |
| D-1, D-2 lap model | ADR-0002, Track R |
| D-3 `HSLOW` zero-countdown clamp | Track R |
| D-4 animation and sound durations | Phase 6 (listing-derived part), Track R (measured part) |
| D-5 trace counter sampling | Permanent, trace format only |

## Playthrough harness (not source behaviour)

A closed-loop planner (`src/app/dplan.cpp`) records a power-on keystroke script.
A pure power-on run can take hours of HSLOW recovery and dies to a single
scorpion sting at Original Mode incoming damage. Two harness mechanisms exist
so a committed script can still be replayed by `dcli` / ctest:

1. **Checkpoints.** After each cleared level, and before the type-10 and
   type-11 fights, `dplan` writes `Game::snapshot()` to
   `.cache/playthrough/<stage>.snap` (gitignored) and types a real
   `ZSAVE <stage>` so `dcli` can `ZLOAD`. The keystroke log beside the snap
   stays one file from power-on.
2. **Fudge factor (D-12).** Script lines `FUDGE incoming <percent>` and
   `FUDGE rest` are not command-parser tokens. `dcli` and `dplan` apply them
   through `Game::set_incoming_damage_percent` and `set_player_damage(63)`.
   100 is Original Mode; 25 is the proof setting.

These are labeled **not source behaviour**. They do not change Original Mode
combat unless a `FUDGE` line is replayed.

## Open

| Item | Label |
|---|---|
| `OCBPTR` rewind in the ring riddle | [OPEN] The core has no object-allocation pointer, and nothing allocates an object after the riddle. |
| Drawing the death and winner screens | Phase 6 |

## Playthrough status

Measured by `dplan` on `cursor/phase-5-playthrough`. Not a WINNER script yet.

| Stage | Fudge | Notes |
|---|---|---|
| Power-on through cleared level 0 | 100 | Wooden/iron, VULCAN → FIRE. Checkpoint `cleared-0`. |
| Cleared level 1 | 100 | RIME → ICE, bronze, revealed IRON in hand. Checkpoint `cleared-1`. |
| Level 2 scorpions | 100 | Revealed IRON (pho 40) under a live torch, hit-and-run. Image left alive. Checkpoint `cleared-2` / `pre-image`. |
| Type 10 / ENDGAM | 25 | Original incoming one-shots the landing. `FUDGE incoming 25` plus `FUDGE rest`. Checkpoint `endgam`. |
| Cleared level 3 | 25 | JOULE → ENERGY, ELVISH. Checkpoint `cleared-3`. |
| Level 4 hole | 25 | Still dying: climb lands on/near balrogs at high CMOVE rate. No WINNER script committed. |

`ctest` has no playthrough replay until a real run emits `WINNER` twice.
