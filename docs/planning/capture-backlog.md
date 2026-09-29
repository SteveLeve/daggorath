# ROM capture backlog

Track R's work list (ADR-0003). Each row names the rule a capture would promote
to ROM-observed or refute. Phases append rows; Track R marks them captured with
a link to the reconciliation entry. Procedure: `archaeology/phase-0b/traces/README.md` §3.

| ID | Capture | Rule(s) at stake | Added by | State |
|---|---|---|---|---|
| C-01 | Phase 0b scripts `t1`–`t5` | parser, keyboard buffer overrun, MOVE/TURN/LOOK timing | Phase 0b | captured — reconciliation §1. `t3` and `t5` stay reference-only |
| C-02 | Level entry at `SECOND` = 0, 1, 7, 30, 59 | fixed maps; `DGEN90` 256-spin at 0; creature placement | Phase 1 | captured, harness-modified — §1 |
| C-03 | Level re-entry | `CMXLND` persistence; `NEWLVL` rebirth | Phase 1 | captured — §1 |
| C-04 | Five-minute `CREGEN` boundary, and the opening lap | `CREGEN` increments without birth | Phase 1 | partial — opening lap captured; five-minute reschedule unresolved — §1, §5 |
| C-05 | Simultaneous expirations | queue tie order; D-1, D-2 | Phase 0b | captured — §1. Same-queue FIFO was not in the sample |
| C-06 | MOVE/TURN animation and `SNOISE` durations | D-4 | Phase 0b | measured — §1. D-4 stays a core deviation |
| C-07 | Blocked MOVE exertion | exertion on blocked path | Phase 1 | partial — weight 35 only — §1 |
| C-08 | `SNOISE` effect on `SEED` | RNG stream after sound | Phase 0b | captured — `SEED` unchanged. A creature sound was not separately captured |
| C-09 | First creature move after level entry | `CBIRTH` queuing of `CMOVE`. Source trace: level 0, vipers (`CMOVE-6` onward) ready at jiffy 85 | Phase 2 | partial — [track-r reconciliation](../archaeology/track-r/reconciliation.md#c-09--first-creature-move-after-level-entry): dispatch timing does not establish queue delay or batch grouping; C-21 alignment and TCB/CBIRTH instrumentation needed |
| C-10 | Two creatures and a keystroke expiring in one jiffy | ADR-0002 lap policy. Source order: jiffy queue (`PLAYER`) before `Q.TEN` (`CMOVE`) | Phase 2 | attempted, blocked — [track-r reconciliation](../archaeology/track-r/reconciliation.md#c-10--two-creatures-and-a-keystroke-expiring-in-one-jiffy-attempted-blocked): core-predicted ties don't reproduce in the ROM and shift under a retargeted keystroke, filed as #43 |
| C-11 | 10-minute idle run on level 0 | `CMOVE` priorities in aggregate. Core trace: `docs/archaeology/phase-2/traces/idle-10min.trace` | Phase 2 | captured — [track-r reconciliation](../archaeology/track-r/reconciliation.md#c-11--10-minute-idle-run-on-level-0): aggregate dispatch counts diverge (PLAYER, CMOVE, HSLOW, LUKNEW), filed as #39 |
| C-12 | Hit/miss sequence against a spider with the wooden sword | `ATTACK`, `DAMAGE`, `SCAL16` | Phase 3 (planned) | not captured |
| C-13 | Faint and recovery timing | `HUPDAT` hysteresis, keyboard suspension | Phase 3 (planned) | not captured |
| C-14 | Torch burn-out across minute boundaries | `BURNER` | Phase 4 (planned) | not captured |
| C-15 | Bare `CLIMB` | manual/source discrepancy | Phase 0 | not captured |
| C-16 | Screenshots at fixed states per level; map modes | viewer and mapper | Phase 6 (planned) | not captured |
| C-17 | Death wizard fade-in and faint fade-out, jiffies per step | `WIZIX`/`WIZZES`, `HUPD30`, `HUPD42`; D-14 | Refinement loop 1 | captured, harness-modified (`PDAM` poke) — phase-7 reconciliation §C-17 |
| C-18 | Faint followed by revival, jiffies per `HUPD42` step | `HUPD42`, D-14 | Refinement loop 2 (captured in loop 3) | captured, harness-modified (`PDAM` poke) — phase-7 reconciliation §C-18 |
| C-19 | Creature picks up an object in view and out of view; creature steps onto the player's cell | `CMOVE` `PUPDAT`/`SYNC` foreground cost, next-task timing | Refinement loop 4 | not captured |
| C-20 | Screen dump with the map up (seer and vision scrolls) | whether the status and text bands show over `MAPPER`; phase-7 reconciliation | Refinement loop 15 | not captured |
| C-21 | Start-up alignment: power-on to the first `PLAYER` dispatch, the first `CMOVE`, `HSLOW` | why the ROM's first `PLAYER` run comes at isr 13–14 against jiffy 1 in the core; the `GAME50` `INIVU` `SYNC` (Q4, D-15); C-09 | Refinement loop (Q4) | not captured |
| C-22 | Level-build time for CLIMB to levels 2, 3 and 4 (and ENDGAM's level 3) | timing | Refinement run 22 | captured 2026-09-27, harness-modified (position and `LEVEL` pokes) — phase-7 reconciliation §C-22 |
