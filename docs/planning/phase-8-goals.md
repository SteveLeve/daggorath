# Phase 8.6 wrap-up goals (PR #32)

A goal list for finishing PR #32 (`phase-8/sdl-platform-wiring`). Each run takes
the first `todo` goal, and only that goal. Statuses: `todo`, `done <commit>`,
`blocked: <reason>`, `needs-steve`.

## Per-run prompt

1. **Orient.**
   - Work in the worktree `../daggorath-phase-8-sdl-wiring`.
   - If the tree is dirty with changes that are not ours, stop.
   - Run `git pull --ff-only`. If `main` has moved, merge it in; rebase only if Steve
     asks.
   - `third_party` is a local symlink to the main checkout's pinned sources. Never
     `git add` it. Stage paths explicitly.
2. **Take one goal:** the first `todo` below.
3. **Gate.**
   - `make all`, judged by its exit code.
   - `boundary-checker` if anything under `src/` changed.
   - `evidence-auditor` if a doc or behaviour changed.
   - Check that every new test fails with the change reverted.
4. **Record, commit and push.**
   - Set the goal's status and add a run-log line.
   - Commit in sentence style and push to PR #32.
5. **Stop and notify** when:
   - the next goal is `needs-steve`,
   - the gate fails and can't be fixed quickly,
   - or no `todo` is left.

   The loop never merges the PR.

## Goals

1. **Verify the merge of `main` (e28d38a).**
   - Acceptance: `make all` passes; crisp does not paint over main's new screens.
   - Status: `done` (checked by reading the code; the on-screen check is deferred to goal 4).
   - The merge exposed two bugs, both fixed:
     - Crisp's overdraw hid the PREPARE! (PREPAX) and EXAMINE pages.
     - Crisp ignored the NLVL50 polarity, so odd levels were drawn white-on-black
       where the pixel style inverts them. The fix applies the view rule
       (source-proven) and the map rule (`[INF]`, raster.hpp).
2. **Headless system-menu state machine.**
   - Status: `todo`.
   - Move `MenuMode` (Top/ChooseSave/ChooseLoad, Y/N confirmations, Esc backing
     out one level) out of `sdl_app.cpp` into a unit with no SDL dependency, in
     the same style as `OverlayBridge`.
   - Acceptance: tests covering the overwrite, Restart and Quit confirmations,
     Esc back-out, and the slot 1–5 round trip.
3. **Pause in the trace.**
   - Status: `todo`.
   - Acceptance: a test shows that a shell pause/resume emits `PAUSE`/`RESUME`
     shell lines and does not advance the core clock (phase-8-plan item 8).
4. **Hand test.**
   - Status: `needs-steve`, once goals 2–3 are done.
   - Steve runs `./build/src/app/dod` (tablet and `--layout=phone`) and checks:
     - Esc pause and resume;
     - the S/L/X/Q menu, slots 1–5, Y/N and Esc;
     - the F1 pixel/crisp toggle on an even and an odd level, including
       PREPARE! during a climb and EXAMINE;
     - tapping the on-screen buttons;
     - a Save → Load round trip.
5. **Video/Controls menu entries.**
   - Status: `todo`, after goal 4.
   - A Video entry that selects the render style (the F1 toggle) and a Controls
     entry that selects the layout.
   - Record them in the phase-8 reconciliation addendum.
6. **Ready to merge.**
   - Status: `todo`.
   - Update the PR body and `phase-8-plan.md`, then ask Steve to merge.

Out of scope:
- The Phase 9 backgrounding hook.
- Crisp line thickness and smoothing (8.3, still open).
- `src/core`.

## Run log

- 2026-09-27, goal 1:
  - `make all` passed on e28d38a.
  - Fixed crisp overdraw on PREPARE!/EXAMINE and crisp polarity. Verified by
    reading the code; the on-screen check is part of goal 4.
