# Refinement loop — per-run prompt

Each run of the loop follows this prompt once. Runs are expected to take 10–30 minutes.
Every run starts from `docs/planning/refinement-log.md`; do not rely on conversation history.
`CLAUDE.md` and `docs/project-instructions.md` override anything here.

Goal: find where `src/` is missing or getting wrong behaviour of the original listing
(`third_party/dod-asm/*.ASM`), fix what is provable, and ask about the rest.

Working branch: `refinement/playthrough-and-discovery`. PR #21 (`cursor/desktop-demo-visible`)
was merged to `main` on 2026-09-27 after runs 1–16.

## 1. Orient (under 2 minutes)
- Run `git status`. If the tree has uncommitted changes that this loop did not make,
  stop, send a PushNotification, and end the run. Changes left by an earlier run of
  this loop (for example, waiting on an audit) are this loop's: finish those first.
- Run `git pull --ff-only` on the working branch.
- Read the log. Apply any answers Steve has written under "Open questions", then move
  those questions to "Answered".
- If the previous run committed without an `evidence-auditor` pass, audit that commit
  before starting new work.

## 2. Pick 1–3 targets
Take them in this order:
1. Entries under "Reported symptoms".
2. `needs-human` rows that Steve has now answered, and `gap-open` rows a run can close.
3. The next `unreviewed` rows in coverage-map order. When the only one left is
   "remaining .ASM files", split one file into label rows and work those.

## 3. Discover
For each target:
- Read the ASM line by line. List every effect: RAM variables written, sounds queued,
  scheduler or timer changes, text output, and edge cases (value clamps, carry wrap,
  fall-through branches, early exits, a `BSR` that falls through into a second call).
- Find the counterpart in `src/` and check each effect against it. Check the display
  side too: a routine can be right in the core and never drawn in the window (runs 15
  and 16 found the map and examine screens missing that way).
- Every `PUPDAT` (and anything that reaches one: `COMUPD`, `INIVU`) costs a `SYNC`
  jiffy under D-15. Check that each caller charges it.
- Write a small scratch harness (`g++ -std=c++20 -Isrc/core/include -Isrc/presentation/include
  <file> build/src/presentation/libdaggorath_presentation.a build/src/core/libdaggorath_core.a`)
  to confirm a suspected bug before fixing it. Keep it in the scratchpad, not the repo.
- Optional cross-checks:
  - The Hunerlach-lineage port in `~/projects/DungeonsOfDaggorath`. It is only a hint
    or a second opinion; never copy it. See `docs/provenance/ledger.md`.
  - `tools/compare/drive-web.mjs` and `tools/compare_port_creatures.py` for scripted
    side-by-side runs.
- Timing questions: look first for existing ROM captures in `captures/` (t1–t5, C-17,
  C-18 and others). If none fits, run one: `. tools/rom/env.sh`, `tools/rom/assemble.sh`,
  then `tools/rom/run-capture.sh <script>` with `DOD_ROM` pointing at the retail `.ccc` in
  `third_party/`. `DOD_POKE="isr:SYMBOL:value[:width],..."` sets up a state
  (harness-modified; say so). Add any symbol you need to `tools/rom/watchlist.tsv`.

## 4. Classify each difference
- **Bug**: our code disagrees with the listing. Fix it (§5).
- **Deviation**: already recorded in `clock-and-scheduler.md` §13. Mark it `deviation`.
- **Quirk**: the original behaviour looks odd but is real. Record it in
  `docs/specification/quirks.md` and preserve it.
- **Unclear**: needs a judgement call. Mark it `needs-human`, add a question to the
  log, and send a PushNotification.

## 5. Patch (bugs only)
- Keep each fix small and give it a regression test in `tests/conformance/` that
  cites `FILE.ASM` and the label or line.
- Label every claim source-proven, ROM-observed, inferred or unresolved, in code
  comments and docs alike.
- Never invent fixture values. A fixture or baseline change needs a reason recorded in
  the relevant `reconciliation.md` before regenerating. Changes of a kind Steve has
  already accepted (D-15 `PUPDAT` charges, Q2) may be regenerated with that record.
  Anything else that moves a baseline, or breaks a test whose value came from our own
  core, goes on a side branch (`refinement/<topic>`) with a question, not onto the
  working branch.
- A test value produced by the core itself (not the listing or a ROM run) is not
  evidence. If a fix breaks one, say so and replace the check with a source-based one.
- Make at most 3 fixes per run, and stop starting new work at about 25 minutes.
- Do not start creature AI, combat or UI redesign, refactors, or new modes. Ask first.

## 6. Gate
- `make all` must pass. Gate on its exit code (`make all && git commit ...`), never on a `grep` of its output: run 20 pushed a failing test that way.
- Run the `evidence-auditor` agent on the diff. If `src/` module boundaries or CMake
  wiring changed, run `boundary-checker` at the same time, in parallel.
- Apply the audit's blocking items, then the cheap notes (labels, line citations,
  missing tests), before committing. Leave the rest as a note in the log.
- If a fix cannot pass within the run, revert it and log the row as `gap-open` with
  the reason.
- Python edits in scripts: when a string replacement fails its assertion, stop and
  re-read the file. Never commit after a partly applied edit (runs 1 and 6 did).

## 7. Record, commit, push
- Update the coverage rows and add one line to "Run history": date, loop number,
  targets, findings, fixes, commits, and any "not run: <reason>".
- Commit each fix separately, plus one commit for the log. Commit messages are one
  sentence in the repo's style.
- `git push origin refinement/playthrough-and-discovery`. If the branch has no open PR
  yet, open one as a draft against `main`. No force-push, no rebase.

## 8. Ping Steve (PushNotification) when
- a question blocks progress,
- the gate fails and cannot be resolved, or
- two consecutive runs find nothing. Suggest stopping the loop.
