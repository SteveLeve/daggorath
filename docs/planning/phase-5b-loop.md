# Phase 5b search loop — per-run prompt

**Closed 2026-09-28.** A candidate reached WINNER and qualified; the winner
ctest (`playthrough_power_on_to_winner`) is enabled, not disabled, and closure
(re-enable, hash reason, reconciliation, audits, `make all`) is done. This file
is kept for its method, not as an open task list — do not start a new loop run
against it without first checking `phase-5b-search-log.md`'s Frontier entry.

Each run of the loop follows this prompt once and tests **one hypothesis**.
Every run starts from [`phase-5b-search-log.md`](phase-5b-search-log.md); do not
rely on conversation history. `CLAUDE.md`, `docs/project-instructions.md` and the
[active prompt](../prompts/phase-5b-honest-playthrough.md) override anything here.
The prompt owns acceptance; this file owns only method and cadence.

Worktree: `~/projects/daggorath-phase-5b-honest-playthrough`, branch
`refinement/phase-5b-honest-playthrough` (draft PR #33). Never rebase or force-push.

## 1. Orient (under 2 minutes)
- `git status`. Changes this loop did not make: stop, PushNotification, end the run.
  Changes left by an earlier run of this loop are this loop's: finish or revert them.
- `git pull --ff-only`. Read the search log: Frontier, Hypothesis queue, Answered.
- `make build`. If it fails on code this loop did not touch, stop and notify.

## 2. Pick
Take the top hypothesis in the queue. If Steve answered an open question, apply it
first. Write down, before running anything, what result would confirm or refute it.

## 3. Probe cheaply, then run a candidate
- Prefer a scratch probe: a throwaway harness (in `$CLAUDE_JOB_DIR/tmp` or
  `build/search/`) that replays a recorded prefix from `build/search/*/cache/*.script`
  into a scratch `Game` and measures the question directly. Scratch state never
  feeds a candidate.
- Candidate: make one planner change in `src/app/dplan.cpp`, then

  ```sh
  make build
  R=build/search/L<run>-<letter>; mkdir -p $R
  timeout 1800 build/src/app/dplan --script $R/candidate.script --cache $R/cache \
      --max-jiffies <bound> > $R/dplan.out 2>&1
  J=$(grep -o 'jiffy=[0-9]*' $R/dplan.out | tail -1 | cut -d= -f2)
  build/src/app/dcli --script $R/candidate.script --jiffies $((J+1)) --trace $R/candidate.trace
  python3 tools/summarize_candidate.py $R/candidate.trace > $R/summary.json
  ```

  Long runs go in the background; the loop wakes when they finish. `build/` is
  ignored; `/tmp` is not durable, so keep candidates under `build/search/`.
- A run may try up to three variants of the same hypothesis (letters a, b, c).
- The same restored state and relative input timing reproduce the same failure.
  A retry must change route, equipment or timing.

## 4. Verdict
- Compare with the Frontier: milestone rank first, then (at the same rank) peak
  power, level-4 unkilled damage, fewer deaths, fewer jiffies.
- Better: keep the change. It becomes the new Frontier.
- Not better: `git checkout src/app/dplan.cpp`. Keep the log row and the lesson.
- A planner change that only fixes a stall or crash counts as better if the
  candidate then gets further, even at the same rank.

## 5. Rules and gate
- No rule changes, FUDGE, snapshot recovery, fixture/baseline/hash/manifest edits,
  or `make all` (it regenerates evidence). The winner ctest is enabled and must
  keep passing; a run that would break it reverts instead.
- Core (`src/core`) edits only for a source-proven bug found while searching: cite
  `FILE.ASM` label, add a regression test, run `evidence-auditor` (and
  `boundary-checker`). Otherwise record it as a question.
- A mechanic the planner relies on is checked against the listing
  (`third_party/dod-asm`, or `~/projects/daggorath/third_party/dod-asm` read-only)
  and labelled source-proven / core-observed / inferred in Facts.
- Strategy guides are strategy-only; record any you rely on in
  `docs/provenance/ledger.md`.
- Scripted edits: chain the build on the edit's success (`python3 edit.py && make build`).
  Run 1 rebuilt stale code after a failed assertion and misread the result.
- Gate: `make build && (cd build && ctest --output-on-failure)` on exit code.
- Planner diffs: run the `playthrough-reviewer` agent on the diff before committing;
  apply blocking items, log the rest. It is defined in this worktree's
  `.claude/agents/playthrough-reviewer.md`; a session started in the main checkout
  does not discover it, so give that file's instructions to a general-purpose
  agent and say so in the log row.

## 6. Record, commit, push
- Add one row to the Candidate table, update Frontier, pop/push the Hypothesis
  queue (what did this run teach; what is now most likely to move the Frontier),
  and add Facts with labels.
- One commit for a kept planner change, one for the log. One-sentence messages in
  the repo's style. `git push origin refinement/phase-5b-honest-playthrough`.

## 7. Stop or ping (PushNotification)
- **WINNER in a candidate:** run
  `python3 tools/verify_playthrough.py --dcli build/src/app/dcli --script $R/candidate.script --jiffies <J+margin> --timeout <measured×3>`.
  Record the result, stop the loop, and notify. Closure (re-enabling the ctest,
  hash reason, reconciliation, audits, `make all`) is a supervised session.
- Two consecutive runs without a Frontier improvement: notify with the next idea;
  the queue probably needs Steve's input.
- A blocking question, a gate failure that cannot be fixed within the run, or
  foreign changes in the tree.
