# ADR-0002 — Scheduler lap model once creature tasks run

**Status:** Proposed. To be settled in Phase 2.

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
3. **Model the lap exactly in order but not in cost**: within one jiffy,
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

## Consequences

- Phase 2's gate includes a same-jiffy multi-creature fixture whose expected
  order is derived from the listing and labelled.
- Track R's first priority capture after the Phase 0b scripts becomes
  "two creatures expiring in the same jiffy as a keystroke".
