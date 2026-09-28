# Touch-overlay refinement log

This log goes with `touch-overlay-loop.md`.

**Statuses:**

| Status | Meaning |
|---|---|
| `todo` | Not started. |
| `fixed <run>` | Fixed in that run. |
| `needs-steve` | Waiting for an answer to an open question. |
| `blocked: <reason>` | Cannot proceed, for the reason given. |

## Reported symptoms (Steve)

Add what you see on screen here. The loop takes these first.

- S1 (2026-09-28): state transitions do not return to the right place after an
  action. *Needs specific repro steps: which button, what you expected, and what
  happened instead.*
- S2 (2026-09-28): the layout should match the mockups.
- S3 (2026-09-28): the overlay must know the game state to offer the right
  options.
- S4 (2026-09-28): Get and Pull should list the actual items by name, in a menu
  next to the Get/Pull button.

## Backlog

Code paths are in `src/`.

| id | Board / source | Code | Status | Note |
|---|---|---|---|---|
| T1 | README "shown when" | `platform/overlay_bridge.cpp` `overlay_state_from` | fixed 1 | `OverlayState` now comes from the game: hands, ring in hand, climb (VFIND), and floor/pack names. |
| T2 | HandStates | `sdl_app.cpp` | fixed 1 | `climb_available` was hard-coded to false, so C never appeared. |
| T3 | Picker | `input/touch_overlay.cpp` `picker_choices` | fixed 1 | Pickers list the real floor and pack names instead of GENTAB's six generic names. |
| T4 | HandStates | `picker_choices` | fixed 1 | The ≡ menu offers I only when that hand holds a ring. |
| T5 | Picker | `sdl_app.cpp` | fixed 1 | Choices showed only their first letter (SCROLL, SHIELD and SWORD all read "S"); they now show the full name. |
| T6 | README "sequential entry" | `sdl_app.cpp` | fixed 1 | A tap outside an open picker now closes it. Before, an empty or unwanted picker was a dead end. |
| T7 | Picker | `sdl_app.cpp` `picker_choice_rects` | fixed 2 | The picker should be a vertical menu anchored beside the button that opened it (left 68 / top 68 / w 200). The title is T18. |
| T8 | Popup | `picker_choice_rects` | fixed 2 | The hand menu should be a row of letters with captions beside the ≡ that opened it. |
| T9 | Climb | `picker_choice_rects` | fixed 2 | The U/D choices should sit beside the C button. |
| T10 | Picker | `sdl_app.cpp` drawing | fixed 2 | The button that opened a picker should be drawn pressed (inverted). |
| T11 | README "sequential entry" | command line | todo | The partial command (e.g. `.G L`) should be echoed while a picker is open. |
| T12 | Incant | `sdl_app.cpp`, `overlay_bridge` | todo | No on-screen QWERTY keyboard: hand-menu I and ⌨ fall back to the physical keyboard. |
| T13 | Main | `sdl_app.cpp` `button_label` | todo | Buttons use placeholder letters; the mockups use ⇤↑⇥ ↶↻↷ ↓ ≡ ⌨ icons. |
| T14 | Main / Legacy | `input/touch_overlay.cpp` layouts | todo | Compare each button's position with the phone and tablet boards. |
| T15 | — | `platform/overlay_bridge.cpp` `handle_tap` | todo | `handle_tap` passes a default `OverlayState{}` to `resolve_tap`. It is harmless while `resolve_tap` ignores the state, but should pass the real one. |
| T16 | touch-input.md §4–5 | docs | todo | §5 still calls GET/PULL/CLIMB UNIMPLEMENTED in the core; the core implements them now. |
| T18 | Picker | `sdl_app.cpp` | todo | The Picker board titles the menu ("Get left: on floor"); no title is drawn yet. |
| T19 | — | `sdl_app.cpp` | todo | An empty floor or pack opens a picker with no rows. Consider showing "nothing here" or not opening it at all (ask Steve if unclear). |
| T20 | — | `sdl_app.cpp` | todo | The hand-menu caption words are defined in `sdl_app.cpp`; they belong with the verb letters in `src/input`. |
| T17 | — | system menu | todo | The Video/Controls menu entries (goal 5 of `phase-8-goals.md`). Out of scope for this loop; tracked here. |

## Open questions for Steve

- Q1 (T3): should the floor picker list objects when the player has no light?
  Today it lists what EXAMINE lists, whatever the light. The README says "none
  in the dark", but `touch-input.md` §4 marks visibility as [INF].
- Q2 (T6): tapping outside a picker now cancels it. Do you also want an
  explicit ✕? No mockup shows one on the picker; the Incant board has one.
- Q3 (S1): which transitions misbehave? For each, give the steps, what you
  expected and what happened.

## Run history

- Run 1, 2026-09-28, T1–T6:
  - The overlay now reads the game state, and the pickers show real names.
  - Tests added: `test_picker_choices_follow_game_state` in
    `tests/input/touch_overlay_tests.cpp` and `test_overlay_state_from_game` in
    `tests/platform/overlay_bridge_tests.cpp`. Reverting the fix makes them
    fail.
  - `make all` passes.
  - Screen check not run: no display.
- Run 2, 2026-09-28, T7–T10:
  - `place_choices` and `picker_anchor` (`src/input`) now put each picker
    beside the button that opened it, using the mockup coordinates:
    - the Picker board's column at x 68,
    - the Popup board's row at 506–722 with captions,
    - the Climb board's U/D at 674.
  - The buttons stay drawn under an open picker, with the opening button shown
    pressed.
  - Also fixed a run-1 bug: choice panels drew a "?" glyph behind each name.
  - Tests are in `test_choices_sit_beside_their_anchor`; they fail when the
    placement is reverted.
  - `make all` passes. Screen check not run: no display.
  - No answers to Q1–Q3 yet.
