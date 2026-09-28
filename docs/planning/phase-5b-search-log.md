# Phase 5b search log

The loop's memory. Method: [`phase-5b-loop.md`](phase-5b-loop.md). Acceptance:
[the active prompt](../prompts/phase-5b-honest-playthrough.md). Earlier
narrative: [`phase-5b-handoff.md`](phase-5b-handoff.md).

Milestone ladder (`tools/summarize_candidate.py`, level indices 0–4):
`start → level-1 → level-2 → image-killed → level-3 → level-4 → wizard-killed → winner`.
At equal rank, compare peak power, then level-4 unkilled damage (WIZ1 power 8000).

## Frontier

- **Best on the current core:** L002-c, `level-4` (rank 5) with level 4 cleared
  except WIZ1: peak power 9660, save WIZARD at jiffy 356701, WIZ1 damage 3608/8000
  (four ring hits of 902). 6 deaths, all recovered. Blocked: 1.2M-jiffy budget in
  `KillWizard` after both rings were spent; the planner loops `PULL RIGHT JOULE
  RING` / `INCANT ENERGY` (~50850 times) with no Joule ring left.
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
| L001-a | idle while `preparing()` (CLIMB build) | image-killed | 4210 | 3 | no clock progress, Loot, lv 3 during ENDGAM's build | superseded by b |
| L001-b | also idle through ENDGAM's NEWLVL 3 (trace: ENDGAM image → RELOCATE) | level-4 | 7539 | 9 | budget 800k, LightUp on lv 4, dead torch | **kept** |
| L001-c | review note: wait to first PLAYER after RELOCATE (+16 jiffies) | level-3 | 5310 | 13 | 12 recoveries from ENDGAM on lv 3 | reverted |
| L001-d | intended revert of c; the edit script failed its assertion, so it rebuilt c | level-3 | 5310 | 13 | same as c (dplan confirmed deterministic) | void |
| L001-e | L001-b window, plus review notes 2–3 (build-wait bound, int type) | level-4 | 7539 | 9 | script byte-identical to L001-b | **kept** |
| L002-a | level-4 stagnation hold in the Clear/search branch | level-4 | 7539 | 9 | same as L001-b: hold never reached (stall was in LightUp) | reverted |
| L002-b | LightUp goes to Clear first on level 4, plus hold | level-4, L4 cleared | 9660 | 6 | budget 1.2M in KillWizard, rings spent | = c |
| L002-c | LightUp goes to Clear first on level 4 (no hold) | level-4, L4 cleared | 9660 | 6 | budget 1.2M in KillWizard; ring re-incant loop | **kept** |

## Hypothesis queue (top first)

1. **WIZ1 after the rings.** In L002-c, KillWizard keeps re-pulling a Joule ring
   that no longer exists. Switch to the Elvish sword (magic channel, ~226/hit at
   power 9660; 4392 damage left ≈ 20 hits) with hit-and-leave, and bound the ring
   branch. Source-check first whether a creature's damage ever decreases (does
   WIZ1 heal between exchanges?) and what WIZ1's hit does to the player.
2. **More ring hits.** Only 4 ring hits landed of the charges carried; check where
   the other charges went (level-4 clearing?) and whether saving them for WIZ1
   gets closer to 8000.
3. **Timing-offset search.** L001-c moved one wait by 16 jiffies and lost a whole
   level: outcomes are highly sensitive to key timing (creature moves and RNG
   draws key off the clock). A cheap search dimension: at a failing stage,
   replay from the last save with small idle offsets (0–60 jiffies) before
   the first command and keep the best. Legal: it changes only key timing.
4. **WIZ1 finishing cycle.** Elvish sword magic channel 249/hit at power 10660
   needs about 11 more hits after cm's 5473. Source-check whether WIZ1 heals
   (creature damage recovery) before relying on attrition; save closer to the fight.
5. **More power at WIZ1.** Farm kills (power ÷ 8 each) and THEWS before ENDGAM;
   the magic channel scales with player power.
6. **Ring allocation** across ENDGAM: hands survive, the reachable bag is cleared.
7. **Strategy guides** for the final fight (ledger-recorded, strategy-only).

## Facts

- **Source-proven** (`DTABAS.ASM`, `PATTK.ASM DAMAGE`): WIZ1 MGO=255 MGD=6 PHO=255
  PHD=0; the Elvish sword's magic channel still deals damage (249 at power 10660).
- **Source-proven** (`PATTK.ASM PATT10`, ENDGAM/EGAM10): charged-ring exertion is
  about half power; ENDGAM sets weight 200, keeps both hands, clears the bag
  except the torch.
- **Source-proven** (`PATTK.ASM PATT42`): a kill adds creature power ÷ 8.
- **Core-observed** (2026-09-27): dplan and dcli replays of a ~190k-jiffy
  candidate each take under a second headless; runs are cheap.
- **Source-proven** (`PCLIMB.ASM PCLI10`): CLIMB UP needs VF.LUP; level 4 has only
  VF.HUP cells, so there is no retreat upstairs from level 4.
- **Source-proven**: each move costs (weight ÷ 8) + 3 exertion (`PTURN.ASM:197-201`);
  ENDGAM sets the weight to 200 (`PATTK.ASM:258-259`); HSLOW removes
  ceil(damage ÷ 64), at least 1, per run (`COMPLR.ASM:71-75`, `game.cpp task_hslow`).
- **Core-observed** (L001-b, L002-a): the level-4 stall was LightUp's rest check
  (damage > 63, never false in L001-b) plus `occupy_tick` fleeing each arrival,
  whose exertion kept damage high.
- **Core-observed** (L002-c): one ring hit on WIZ1 at power ~9660 deals 902.
- **Core-observed** (L000/L001): during a timed NEWLVL (D-19) the new level's
  objects and creatures are not yet placed; a CLIMB shows `preparing()`, but
  ENDGAM's NEWLVL 3 does not, and `ENDGAM wizard` is emitted on WIZ1's death,
  not on level-3 entry (`game.cpp endgame_resume/endgame_wizard`).
- **Core-observed**: `tools/verify_playthrough.py`'s closed vocabulary lacked the
  `NEWLVL` event D-19 added, so it would have rejected every current trace. Fixed
  in run 1 with a test.
- **Core-observed**: scripts from before the D-19 merge do not replay to the same
  places; any old candidate is a strategy hint only.

## Open questions

(none)

## Answered

(none)

## Run history

- 2026-09-27 run 0 (supervised): created this log, `phase-5b-loop.md`,
  `tools/summarize_candidate.py` with tests; baseline L000 at `aab6f49`.
- 2026-09-27 run 1: hypothesis 1 (timed-build stall) confirmed. Planner idles
  through CLIMB and ENDGAM builds; L001-b reaches level 4 (rank 2 → 5). Verifier
  vocabulary gains NEWLVL; summariser levels now from NEWLVL. Review:
  playthrough-reviewer instructions given to a general-purpose agent (native role
  not discoverable from the main-checkout session).
- 2026-09-28 run 2: level-4 stall traced to LightUp's rest check (L002-a's
  search-branch hold was the wrong place and never ran). LightUp now goes straight
  to Clear on level 4; L002-c clears level 4 except WIZ1, power 9660, WIZ1 3608/8000.
