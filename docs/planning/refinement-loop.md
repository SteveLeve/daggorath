# Refinement loop — per-run prompt

Each run of the loop follows this prompt once. Runs are expected to take 10–30 minutes.
Every run starts from `docs/planning/refinement-log.md`; do not rely on conversation history.
`CLAUDE.md` and `docs/project-instructions.md` override anything here.

Goal: find where `src/` is missing or getting wrong behaviour of the original listing
(`third_party/dod-asm/*.ASM`), fix what is provable, and ask about the rest.

## 1. Orient (under 2 minutes)
- Run `git status`. If the tree has uncommitted changes that this loop did not make,
  stop, send a PushNotification, and end the run.
- Run `git pull --ff-only` on `cursor/desktop-demo-visible`.
- Read the log. Apply any answers Steve has written under "Open questions", then move
  those questions to "Answered".

## 2. Pick 1–3 targets
Take them in this order:
1. Entries under "Reported symptoms".
2. `needs-human` rows that Steve has now answered.
3. The next `unreviewed` rows in coverage-map order.

If the coverage map is empty, seed it first. Grep the routine labels (column-1 labels
with an entry comment) in the priority files, then add the remaining files as
file-level rows.

## 3. Discover
For each target:
- Read the ASM line by line. List every effect: RAM variables written, sounds queued,
  scheduler or timer changes, text output, and edge cases (value clamps, carry wrap,
  fall-through branches, early exits).
- Find the counterpart in `src/` and check each effect against it.
- Optional cross-checks:
  - The Hunerlach-lineage port in `~/projects/DungeonsOfDaggorath`. It is only a hint
    for where to look; never copy it. See `docs/provenance/ledger.md`.
  - `tools/compare/drive-web.mjs` and `tools/compare_port_creatures.py` for scripted
    side-by-side runs.

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
- Label every claim source-proven, ROM-observed, inferred or unresolved.
- Never invent fixture values. A fixture hash change needs a reason recorded in
  `reconciliation.md` before regenerating.
- Make at most 3 fixes per run, and stop starting new work at about 25 minutes.
- Do not start creature AI, combat or UI redesign, refactors, or new modes. Ask first.

## 6. Gate
- `make all` must pass.
- Run the `evidence-auditor` agent on the diff.
- If `src/core`, `src/input`, `src/platform` or the CMake wiring changed, also run
  `boundary-checker`.
- If a fix cannot pass within the run, revert it and log the row as `gap-open` with
  the reason.

## 7. Record, commit, push
- Update the coverage rows and add one line to "Run history": date, loop number,
  targets, findings, fixes, commits, and any "not run: <reason>".
- Commit each fix separately, plus one commit for the log. Commit messages are one
  sentence in the repo's style.
- `git push origin cursor/desktop-demo-visible` updates draft PR #21. No force-push,
  no rebase, no new PR.

## 8. Ping Steve (PushNotification) when
- a question blocks progress,
- the gate fails and cannot be resolved, or
- two consecutive runs find nothing. Suggest stopping the loop.
