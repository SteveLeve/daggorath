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
   - Status: `done`. `dag::shell::MenuState` (src/shell/include/daggorath/system_menu.hpp),
     tests/shell/system_menu_tests.cpp (27 checks; mutating back-out or N-cancel
     fails them).
   - Move `MenuMode` (Top/ChooseSave/ChooseLoad, Y/N confirmations, Esc backing
     out one level) out of `sdl_app.cpp` into a unit with no SDL dependency, in
     the same style as `OverlayBridge`.
   - Acceptance: tests covering the overwrite, Restart and Quit confirmations,
     Esc back-out, and the slot 1–5 round trip.
3. **Pause in the trace.**
   - Status: `done`, already covered: tests/shell/shell_tests.cpp
     test_pause_invariance checks the PAUSE/RESUME lines, the no-op ticks and an
     identical core trace. No change needed.
   - Acceptance: a test shows that a shell pause/resume emits `PAUSE`/`RESUME`
     shell lines and does not advance the core clock (phase-8-plan item 8).
4. **Hand test.**
   - Status: `done`. Steve's hand test found real gaps in the touch UI
     (state transitions, layout, game-state awareness); these ran through
     the separate touch-overlay loop (`docs/planning/touch-overlay-loop.md`,
     `touch-overlay-log.md`, runs 1–7), not this goal list. The backlog is
     now clear of `todo` rows: T17 stays explicitly out of that loop's
     scope, and T25 (save-slot delete) is deferred to the later "Tape"
     screen, issue #34.
5. **Video/Controls menu entries.**
   - Status: `done`. `V VIDEO: PIXEL/CRISP` and `C CONTROLS: TABLET/PHONE`
     on the system menu's Top screen (`src/platform/sdl_app.cpp`), handled
     directly rather than through `MenuState` since both are
     presentation-only. `C` resizes the live window
     (`OverlayBridge::set_layout` plus a window/renderer/texture rebuild —
     see the reconciliation addendum for why a plain `SDL_SetWindowSize`
     wasn't enough). Recorded in `docs/archaeology/phase-8/reconciliation.md`
     §8.6.7.
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
- 2026-09-27, goals 2–3:
  - Menu key handling moved from `sdl_app.cpp` into `MenuState`; `make all`
    passes.
  - Goal 3 was found already tested. Next: goal 4 (`needs-steve`).
- 2026-09-28, goal 4:
  - Closed out through the touch-overlay loop rather than this list; see
    `touch-overlay-log.md` runs 1–7. Next: goal 5 (Video/Controls menu
    entries), still `todo`.
- 2026-09-28, goal 5:
  - Added `V`/`C` to the system menu's Top screen. `C`'s live layout switch
    needed a window rebuild, not just `SDL_SetWindowSize` — see the
    reconciliation addendum §8.6.7 for what didn't work and why.
  - `make all` passes, 23/23. Verified on screen through `--shots` with
    injected key presses, both toggle directions, plus a move afterward to
    confirm the game still runs post-rebuild.
  - Next: goal 6 (ready to merge).
