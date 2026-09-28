# Phase 5b search log

The loop's memory. Method: [`phase-5b-loop.md`](phase-5b-loop.md). Acceptance:
[the active prompt](../prompts/phase-5b-honest-playthrough.md). Earlier
narrative: [`phase-5b-handoff.md`](phase-5b-handoff.md).

Milestone ladder (`tools/summarize_candidate.py`, level indices 0–4):
`start → level-1 → level-2 → image-killed → level-3 → level-4 → wizard-killed → winner`.
At equal rank, compare peak power, then level-4 unkilled damage (WIZ1 power 8000).

## Frontier

- **WINNER on the current core:** L003-c, planner at run 3. `dplan --max-jiffies
  1600000` reports won=1 at jiffy 381124 (179036 keys, script SHA-256
  `54a2cc13…d049`). Saves POWERON, FLOORA, FLOORB, FLOORC, IMAGE, ENDGAM, FLOORD,
  WIZARD; 6 deaths, each followed by RESTART and ZLOAD of the latest save; peak
  power 10660; WIZ1 killed at jiffy 381100; WINNER at 381122.
- **Independent qualification** (`tools/verify_playthrough.py`, two fresh default
  `dcli` replays, byte-identical): at `--jiffies 381124`, trace SHA-256
  `1628c7c8c99e2cb3c10981ecca7f53ac437a3b7fd299fbad310b77783cd87be4`; at any
  `--jiffies` ≥ 381200 (tried 381200, 382000) the trace ends at jiffy 381139 and its
  SHA-256 is `b56777eecb2c887c565bd0b55add00ed099e9ac085f3c3e6d28b0e7b753661d1`
  (use this with margin). One replay takes about 8 s here. This proves deterministic
  core replay under the documented deviations, not ROM conformance.
- **Closure (2026-09-28, supervised):** script published at
  `docs/archaeology/phase-5/traces/power-on-to-winner.script` (regenerated at
  `5ac2c1b`, byte-identical to L003-c); replacement reason recorded first in
  phase-5 reconciliation; `playthrough_power_on_to_winner` re-enabled through
  `tools/verify_playthrough.py --jiffies 382000 --timeout 120 --sha256 b56777ee…`;
  §13 D-12 independence note; `make all` passed (21/21 tests, 56 fixtures, nothing
  else regenerated). Reviews: boundary-checker and playthrough-reviewer (no
  blocking findings); evidence-auditor found two blocking §13 items (stale D-18 row;
  the Phase 5b restart section lost in merge `aab6f49`), both fixed and re-audited
  clean. `make all` re-run after the fixes: 21/21, 56 fixtures. Phase 5b complete.
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
| L003-a | sword on WIZ1 when no ring ready, damage < power/2; ring pull only if owned | wizard-killed | 10660 | 6 | GET typed before the queued leave move ran; INCANT FINAL failed | base for c |
| L003-b | as a, damage < power/3 | wizard-killed | 10660 | 6 | same; WIZ1 dies 32k jiffies later | dropped |
| L003-c | a + TakeSupreme idles 10 jiffies before trusting position | **winner** | 10660 | 6 | — | **kept, qualified** |

## Hypothesis queue (top first)

Search is complete; the loop stopped at the WINNER. Remaining work is closure
(see Frontier). Optional later: shorten the route (fewer deaths or jiffies) only
if closure review asks for it.

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
- **Source-proven** (listing search): creature damage is written only by
  `PATTK.ASM DAMAGE` (accumulates; P.CCDAM/P.ATDAM offset 10, no other writer), so
  WIZ1 does not heal; only a ZLOAD restores the saved value.
- **Core-observed** (L003): after the rings, Elvish-sword hits finish WIZ1.
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
- 2026-09-28 run 3: sword on WIZ1 after the rings, bounded ring re-pull, and a settle
  wait before taking SUPREME. L003-c reaches WINNER at jiffy 381122 and qualifies
  under the independent verifier. Loop stopped for supervised closure.
- 2026-09-28 closure: steps 5–7 of the prompt done in a supervised session; see Frontier.
