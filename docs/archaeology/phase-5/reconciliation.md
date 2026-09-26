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
| D-1, D-2 lap model | ADR-0002, Track R |
| D-3 `HSLOW` zero-countdown clamp | Track R |
| D-4 animation and sound durations | Phase 6 (listing-derived part), Track R (measured part) |
| D-5 trace counter sampling | Permanent, trace format only |

## Open

| Item | Label |
|---|---|
| `OCBPTR` rewind in the ring riddle | [OPEN] The core has no object-allocation pointer, and nothing allocates an object after the riddle. |
| Drawing the death and winner screens | Phase 6 |
