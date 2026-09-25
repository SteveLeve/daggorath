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
| C-09 | First creature move after level entry | `CBIRTH` queuing of `CMOVE` | Phase 2 (planned) | not captured |
| C-10 | Two creatures and a keystroke expiring in one jiffy | ADR-0002 lap policy | Phase 2 (planned) | not captured |
| C-11 | 10-minute idle run on level 0 | `CMOVE` priorities in aggregate | Phase 2 (planned) | not captured |
| C-12 | Hit/miss sequence against a spider with the wooden sword | `ATTACK`, `DAMAGE`, `SCAL16` | Phase 3 (planned) | not captured |
| C-13 | Faint and recovery timing | `HUPDAT` hysteresis, keyboard suspension | Phase 3 (planned) | not captured |
| C-14 | Torch burn-out across minute boundaries | `BURNER` | Phase 4 (planned) | not captured |
| C-15 | Bare `CLIMB` | manual/source discrepancy | Phase 0 | not captured |
| C-16 | Screenshots at fixed states per level; map modes | viewer and mapper | Phase 6 (planned) | not captured |
