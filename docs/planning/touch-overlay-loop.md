# Touch-overlay refinement loop: per-run prompt

Each run finds and fixes gaps between the touch overlay and the mockups in
`docs/design/touch-controls/mockups/` (Main, Popup, Picker, Legacy, HandStates,
Incant, Climb) and the README in the same folder. Progress, the backlog and
Steve's questions live in `docs/planning/touch-overlay-log.md`.

Work in the worktree `../daggorath-phase-8-sdl-wiring`, branch
`phase-8/sdl-platform-wiring` (PR #32). `third_party` there is a local symlink to
the pinned sources. Never `git add` it; stage paths by name.

## One run

1. **Orient** (2 minutes or less).
   - If the tree is dirty with changes that are not ours, stop and notify Steve.
   - Run `git pull --ff-only`. If `main` has moved, merge it in; rebase only if
     Steve asks.
   - Read the log. Apply any answer Steve has written under "Open questions", and
     move the answered question to the run history.
2. **Pick one or two items.**
   - Take a row under "Reported symptoms" first.
   - Otherwise take `todo` backlog rows in this order: game state reaching the
     overlay, named pickers, transitions and cancel, placement against the
     mockups, the incant keyboard, icons.
3. **Discover.**
   - Read the mockup board and the README row for the item, then the code.
   - Record every concrete difference you find as a new backlog row, even ones
     you won't fix this run.
   - Screen checks: write a shots script in the scratchpad, never in the repo.
     Each line is one of `<ms> key <SDL key name>`, `<ms> tap <x> <y>`,
     `<ms> shot <file.bmp>` or `<ms> quit`. Then run:

     ```
     SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy build/src/platform/dod [--layout=phone] --shots=<script>
     ```

     Convert each shot with `convert x.bmp x.png` and Read the PNG.
     - Tablet coordinates: button pitch 54, margin 14, 48 square. At 768x576
       the left G is centred at (38, 92).
     - Phone: 1248x576, units scaled by 576/390.
     - Look at every screen you change, in both layouts.
     - Shots follow the wall clock, so they are for looking, never for traces.
4. **Fix.**
   - Put decisions in the headless units: `src/input/touch_overlay.*` for
     choices, layout and resolution, and `src/platform/overlay_bridge.*` for
     game state and taps. Test them in `tests/input/` or `tests/platform/`.
   - `sdl_app.cpp` only draws and routes what those units return.
   - Each new test must fail when its fix is reverted. Check that.
   - At most about 3 fixes per run, and stop at about 30 minutes.
5. **Classify.**
   - If the mockups, README and ADRs don't decide an item, mark it
     `needs-steve`, add a question and move on.
   - Changes to `src/core` (for example a new read-only accessor) always go to
     Steve as a question first.
   - The core is never changed to suit the UI.
6. **Gate.**
   - Run `make all` and gate on its exit code, not on a grep of its output.
   - Run `boundary-checker` when anything under `src/` changes.
   - Run `evidence-auditor` when docs or behaviour change.
   - If the gate fails and can't be fixed quickly, revert, mark the row
     `blocked`, and move on.
7. **Record and push.**
   - Update the backlog rows, add one run-history line, and commit in the
     repository's sentence style.
   - Push to PR #32. Never merge or force-push.
8. **Stop and notify Steve** (PushNotification) when:
   - every remaining row is `needs-steve` or `blocked`,
   - the gate fails,
   - or two runs in a row find nothing new.

Out of scope: core game mechanics, the Video/Controls menu entries, and the
Phase 9 backgrounding hook.
