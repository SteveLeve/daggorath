# Phase 5b implementation handoff

Prepared 2026-09-27 for continuation after context compaction/model switch.
This is execution status, not a second specification. The
[active prompt](../prompts/phase-5b-honest-playthrough.md) owns scope and acceptance;
the [shared skill](../../.agents/skills/daggorath-playthrough/SKILL.md) owns workflow.

## Workspace and state

- Dedicated worktree: `/home/steve-leve/projects/daggorath-phase-5b-honest-playthrough`.
- Branch: `refinement/phase-5b-honest-playthrough`.
- Base: `6459a8486c9af82a3af4b02705363379eab61e57` from
  `refinement/playthrough-and-discovery`. Check current HEAD/status on resumption.
- Preparation files are intentionally uncommitted: prompt, refinement-log decision,
  this handoff, shared skill/Claude adapter, three Codex agents, and the new shared
  playthrough-reviewer instructions. Preserve these as task-owned work.
- No gameplay, planner, fixture, baseline, or CMake changes have been made for 5b.
  The winner ctest remains disabled. No successful honest route is established.
- Do all edits, builds, and generated outputs in this worktree. The original
  checkout and other worktrees can have unrelated concurrent work.

## Decisions already settled

See the prompt rather than reopening these choices: actual death/restart/typed
ZLOAD in the final route; full read-only hidden-state inspection and faithful
exploratory search; headless recovery first; preserve game rules; use the dedicated
worktree. Scratch snapshots are allowed for independent search, never candidate
recovery. Human playability or limited player knowledge is not a requirement.
The user intends Sol for execution; review agents inherit the session model.

## Preparation verification

Completed on 2026-09-27:

- Both skill frontmatters passed the skill-creator quick validator. All three
  Codex agent TOMLs parsed, and shared instruction paths resolved.
- Fresh Codex CLI 0.157.1 discovered the skill and all three custom roles. In a
  persisted read-only session, each role spawned and loaded its shared contract.
  The playthrough reviewer allowed creature-location inspection and rejected
  candidate snapshot recovery and a text-only WINNER check in a hypothetical case.
- Independent review found two wording issues, both corrected and re-reviewed:
  closure review requirements and candidate hash calculation versus regression
  baseline comparison. No further findings in those corrections.
- `make verify`: 18 Phase 0b + 16 Phase 6 + 22 text fixtures, zero problems.
- `git diff --check` passed. Gameplay build/tests and full `make all` have not run
  in this worktree; preparation checks do not establish gameplay correctness.

An ephemeral CLI session discovered the roles but could not spawn them (`no
thread with id`). Retrying with a persisted session succeeded. Do not spend time
re-diagnosing that completed smoke-test issue. If this host cannot discover a
role, the shared skill describes the explicit-instruction subagent fallback.

## First implementation slice

1. Enter this worktree, inspect status, read the shared skill and the authoritative
   documents listed by AGENTS.md/CLAUDE.md, then the active prompt. README identifies
   5b as open despite historical phase text elsewhere. The scaffold is complete;
   proceed to implementation rather than creating another planning framework.
2. Establish a local build/test baseline with `make build && make test`. Record
   the intentionally disabled winner ctest distinctly from passes or failures.
3. Read the pinned listing's DEATH, CLOCK, GAME/COMINI and save/load paths. The new
   worktree currently has neither `build/` nor `third_party/dod-asm`. The original
   checkout has the listing at the exact pin
   `a94326f00ebb16a106b540c58bc2ccf5f7b66dac` (verified at preparation).
   It may be read without modification for archaeology. For normal worktree-local
   `make all`, provision an independent pinned checkout using `make sources`;
   do not share mutable build directories or change the other checkout's listing.
4. Implement and test headless recovery before reworking route strategy. Inspect
   Game::advance_jiffies, update_heart_rate, start, tape_operation, and Scheduler's
   interrupt/run_ready_pass. Currently `halted_` prevents interrupt processing and
   total_jiffies advancement. Resetting RAM must not erase future input, trace
   history, or the external cassette. Audit callback ownership when resetting
   scheduler/tasks; avoid copying callbacks bound to a different Game object.
5. Define and test the restart trace event used by acceptance. Establish the
   triggering key's treatment from the listing, distinguish absolute replay time
   from saved game counters, and document timing inferences. Source-backed fixes
   may change existing death traces; explain those changes before regeneration.
6. Continue through the prompt's planner, strategy research, independent acceptance,
   and evidence closure steps. Keep candidates separate from committed artifacts.
   Keep a concise progress entry here or in the refinement log after each slice,
   with commands/results and the next concrete action.

The existing fixture-change skill orders regeneration before its rationale; the
repository's authoritative preservation rule requires the reason before baseline
regeneration. Follow the authoritative ordering. `make all` regenerates tracked
artifacts, so inspect its diff and do not treat regeneration as automatic approval
of a new baseline. Do not delete unrelated snapshot APIs or expand into desktop
recovery redesign to simplify the headless task.

## Implementation progress — recovery slice (2026-09-27)

Core recovery is implemented and passes the 13 enabled ctests. Winner ctest stays
disabled. A default power-on dcli smoke replay, with typed ZSAVE and ZLOAD and
natural death, matched in two fresh processes (1700 jiffies; save 12, death 1591,
restart 1601, load 1622). Temporary artifacts are `/tmp/phase5b-recovery*`; they
are recovery diagnostics, not a winning candidate. The independent review found
the shared halt also affects victory IRQs; assertion and source documentation
were corrected. Review verification passed 56 fixtures. No baseline regeneration.

Next: rework dplan candidate checkpoints/recovery and remove fudge/snapshot
paths. Its current `type()` batches same-jiffy keys (existing input model);
checkpoint writes snapshot before save, stage names contain digits, resume and
automatic fallback use snapshot/fudge. KillImage/KillWizard still assume old
ring longevity. Keep scratch search separate from candidate execution.

## Implementation progress — legal planner experiments

Recovery/scaffolding committed as `008ab0f`. Planner now emits no FUDGE, uses
confirmed uppercase alphabetic cassette saves, never restores snapshots, and
always starts from power-on. Candidate retries preserve history and change
relative timing; maximum 12 recoveries and requested simulated-jiffy bound.
Death events are detected even if an already queued key restarts automatically.
Planner checkpoints are external bookkeeping, not game-state checkpoints.

Candidates `build/candidate-a` through `candidate-i` are diagnostics only; none
is independently qualified. Candidate h defeated the image at normal damage
with 4085 power after drinking level-2 THEWS, then exhausted fourth-floor retries.
Its image route saves ENDGAM at 93199 with substantial residual exertion; need
to improve transition survival and acquire fourth-floor equipment. Candidate i
changed retreat choice to distance from hazards on level 3 as well as 4; it
also exhausted 12 recoveries at 105736 jiffies. This is a bounded failed search,
not a blocker requiring user input.

**Source-proven** (`DTABAS.ASM CREXXX WIZ0/WIZ1`): wizard physical
susceptibility is zero, so an iron sword cannot finish the image.
**Source-proven** (`PATTK.ASM PATT10`, `ENDGAM/EGAM10`): charged ring
exertion is about half power; ENDGAM sets weight 200 and clears the bag except
for the torch, preserving hands. **Inferred** from candidate-b traces: repeated
pursuit/escape movement can create a nonzero healing equilibrium. The candidate
switches rings before the first spends to retain charges in hand at ENDGAM.

The planner reviewer found startup budget/diagnostic gaps; fixed by clamping all
advance calls to the requested budget and including latest save in failures.
13 enabled ctests pass. Winner remains disabled; baseline untouched. Next slice:
faithful scratch lookahead for fourth-floor survival and independent verifier.
