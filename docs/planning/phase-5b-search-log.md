# Phase 5b search log

The loop's memory. Method: [`phase-5b-loop.md`](phase-5b-loop.md). Acceptance:
[the active prompt](../prompts/phase-5b-honest-playthrough.md). Earlier
narrative: [`phase-5b-handoff.md`](phase-5b-handoff.md).

Milestone ladder (`tools/summarize_candidate.py`, level indices 0–4):
`start → level-1 → level-2 → image-killed → level-3 → level-4 → wizard-killed → winner`.
At equal rank, compare peak power, then level-4 unkilled damage (WIZ1 power 8000).

## Frontier

- **Best on the current core:** L000, `level-2` (rank 2), peak power 1605,
  188568 jiffies, saves POWERON/FLOORA/FLOORB, 3 deaths all recovered.
  Blocked: "planner made no clock progress" in phase 5 (`Loot`) on level 2,
  `live=0`, immediately after `CLIMB level=2`.
- **Best ever, on an older core:** candidate cm (`/tmp`, 2026-09-27 21:09), `level-4`,
  WIZ1 at power 10660, WIZ1 damage 5473 of 8000 before twelve recoveries failed from
  save WIZARD. Its script diverges on the current core (replays only to level 1),
  and the planner edits after `22e618e` that produced it were not committed.

## Candidate table

| id | change | milestone | peak power | deaths | blocked by | verdict |
|----|--------|-----------|-----------:|-------:|------------|---------|
| h–w (older core) | image, L3/L4 survival work; see handoff | level-4 | 7310 | many | L4 population, 12 recoveries | superseded |
| cb, ce (older core, uncommitted planner) | ring-then-sword WIZ1 | level-4 | 10660 | — | no charged ring / no finishing sword attack | superseded |
| cd, cl, cm (older core, uncommitted) | sword cycle at WIZ1 | level-4 | 10660 | 51 (cm) | WIZ1 damage 5473/8000, recoveries exhausted | superseded |
| cf–ck (older core, uncommitted) | WIZ1 variants | level-4 | 10660 | — | jiffy budget, WIZ1 damage 2142–3959 | superseded |
| cq–cv (older core, uncommitted) | level-2 farming / waves / dark | level-3 at best | 4987 | — | recoveries or budget | superseded |
| L000 | none: HEAD `aab6f49` planner on merged core | level-2 | 1605 | 3 | no clock progress, Loot, lv 2, live=0 | baseline |

## Hypothesis queue (top first)

1. **Timed-CLIMB stall.** After the merge of D-19 (CLIMB/ENDGAM level builds take
   the C-22 measured time with PREPARE! on screen), the planner enters `Loot` on
   level 2 with `live=0` and advances no clock. Likely it inspects creatures or
   issues keys before the build completes. Confirm from `build/search/L000`
   (`dplan.out` ticks, trace tail); fix by waiting/advancing through the build.
   Refute if the jiffy counter is stuck for a reason unrelated to the build.
2. **Re-establish the older frontier on the current core.** Placement follows
   `SECOND` at level entry, so D-19 moves every creature. Re-tune route choices
   level by level; expect the image and level-3/4 survival heuristics to need work.
3. **WIZ1 finishing cycle.** Elvish sword magic channel 249/hit at power 10660
   needs about 11 more hits after cm's 5473. Source-check whether WIZ1 heals
   (creature damage recovery) before relying on attrition; save closer to the fight.
4. **More power at WIZ1.** Farm kills (power ÷ 8 each) and THEWS before ENDGAM;
   the magic channel scales with player power.
5. **Ring allocation** across ENDGAM: hands survive, the reachable bag is cleared.
6. **Strategy guides** for the final fight (ledger-recorded, strategy-only).

## Facts

- **Source-proven** (`DTABAS.ASM`, `PATTK.ASM DAMAGE`): WIZ1 MGO=255 MGD=6 PHO=255
  PHD=0; the Elvish sword's magic channel still deals damage (249 at power 10660).
- **Source-proven** (`PATTK.ASM PATT10`, ENDGAM/EGAM10): charged-ring exertion is
  about half power; ENDGAM sets weight 200, keeps both hands, clears the bag
  except the torch.
- **Source-proven** (`PATTK.ASM PATT42`): a kill adds creature power ÷ 8.
- **Core-observed** (2026-09-27): dplan and dcli replays of a ~190k-jiffy
  candidate each take under a second headless; runs are cheap.
- **Core-observed**: scripts from before the D-19 merge do not replay to the same
  places; any old candidate is a strategy hint only.

## Open questions

(none)

## Answered

(none)

## Run history

- 2026-09-27 run 0 (supervised): created this log, `phase-5b-loop.md`,
  `tools/summarize_candidate.py` with tests; baseline L000 at `aab6f49`.
