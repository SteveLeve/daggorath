# ADR-0002 — Scheduler lap model once creature tasks run

**Status:** Accepted, 2026-09-25. See Resolution.

## Context

`clock-and-scheduler.md` §13 records:

- **D-1** the core runs per-jiffy foreground passes instead of the original's
  endless `SCHED` lap;
- **D-2** tasks made ready during a pass run on the next pass;
- **D-6** `CBIRTH` does not queue `CMOVE`, precisely so D-1 and D-2 would not
  interact with creatures before movement was specified.

Phase 2 must queue `CMOVE`, so up to 32 creature tasks join the player task and
the system tasks. The lap model now decides observable event order: which
creature moves first when several expire in the same jiffy, and whether a
creature made ready mid-lap acts before the player's next keystroke is parsed.
No ROM capture exists to measure it (`provenance/rom-diff.md`).

## Options

1. **Keep D-1/D-2.** Deterministic, already tested. Risk: a systematic one-lap
   lag against the original for every creature made ready mid-lap.
2. **Model the endless lap exactly**, bounded by the jiffy: the lap repeats
   until an interrupt boundary, with the source's per-task cost unknown.
   Requires a CPU-cost model the project does not have; any cost table would be
   invented. Rejected unless Track R measures costs.
3. **Preserve lap ordering with a per-jiffy fixed point** (order, not cost): within one jiffy,
   repeat laps until no task is ready (a fixed point), with a guard that a task
   returning `Q.SCD` runs at most once per jiffy. Closer to the source's order
   where CPU time is ample; still an inference about how many laps fit in
   one jiffy.

## Proposed decision

Phase 2 reads `SCHED`, `CMOVE`'s return paths and every `Q.SCD` return in the
listing, then chooses between 1 and 3 on source evidence, recording the choice
here with a **Resolution** section and updating D-1/D-2 labels. Whatever is
chosen, it is one switchable policy in `scheduler.cpp` so a later ROM capture can
flip it without rewriting tasks; Original Mode ships exactly one policy.

## Resolution (2026-09-25)

`SCHED` (`COMMON.ASM`) is an endless lap: the tail returns to the head with no
wait, a task that returns `Q.SCD` stays linked, and any other return is
`QUERMV` plus `QUEADD`. `CMOVE` returns `Q.TEN` on every live path
(`CRETUR.ASM` `CMOV99`). A dead creature returns with `B` still holding
`P.CCUSE` (0), which is not `Q.SCD`. Nothing in this phase returns `Q.SCD`.

Option 2 stays rejected: a lap's CPU cost is not in the listing. Option 1
matches today's tests but drops a task that another task places on `SCDQUE`
during the lap until the next jiffy, which the source would reach when it
walks the tail. Option 3 is the policy Original Mode runs: within one jiffy,
repeat the lap until every task queued onto `SCDQUE` this jiffy has run once;
a task that returns `Q.SCD` is not run a second time in that jiffy. That bound
is inferred. How many source laps fit in a jiffy is unresolved until a capture
measures it (C-10).

Countdown scans follow `QUEADD` link order. After `PLAYER` expires it is
appended behind tasks already on `Q.JIF`, so a later same-scan tie runs
`HSLOW` before `PLAYER`. Cross-queue order is unchanged: the jiffy queue is
scanned before a rollover queue.

## Consequences

- Phase 2's gate includes a same-jiffy multi-creature fixture whose expected
  order is derived from the listing and labelled.
- Track R's first priority capture after the Phase 0b scripts becomes
  "two creatures expiring in the same jiffy as a keystroke".
