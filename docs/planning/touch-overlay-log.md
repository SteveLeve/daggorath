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
- S5 (2026-09-28, fixed run 3): the movement buttons' positions and labels don't match the
  mockup. Every move button reads "M" and every turn "T"; use arrow icons.
- S6 (2026-09-28, fixed run 3): E and L should toggle. E appears only while looking (the
  view), L only while examining (the pack and floor list).
- S8 (2026-09-28, fixed run 6): each hand shows only A and ≡. G and P move
  into ≡, G only when something is on the floor and P only when the pack
  holds something.
- S9 (2026-09-28, fixed run 6): remove the ⌨ button; the keyboard opens only
  for INCANT.
- S7 (2026-09-28, fixed run 4): "K" seems to do nothing. It is the ⌨ Keyboard button (free
  command line); see T12.

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
| T11 | README "sequential entry" | command line | fixed 4 | The partial command (e.g. `.G L`) should be echoed while a picker is open. |
| T12 | Incant | `sdl_app.cpp`, `overlay_bridge` | fixed 4 | No on-screen QWERTY keyboard: hand-menu I and ⌨ fall back to the physical keyboard. |
| T13 | Main | `sdl_app.cpp` `button_label` | fixed 3 | Buttons use placeholder letters; the mockups use ⇤↑⇥ ↶↻↷ ↓ ≡ ⌨ icons. |
| T14 | Main / Legacy | `input/touch_overlay.cpp` layouts | fixed 3 | Compare each button's position with the phone and tablet boards. |
| T15 | — | `platform/overlay_bridge.cpp` `handle_tap` | fixed 5 | `handle_tap` passed a default `OverlayState{}` to `resolve_tap`; it now passes the state `buttons()` last laid out. |
| T16 | touch-input.md §4–5 | docs | fixed 5 | §5 called GET/PULL/CLIMB UNIMPLEMENTED; a dated addendum now records that all 15 verbs are implemented. |
| T18 | Picker | `sdl_app.cpp` | todo | The Picker board titles the menu ("Get left: on floor"); no title is drawn yet. |
| T19 | — | `sdl_app.cpp` | fixed 6 | An empty picker used to open with no rows. Now G and P appear in ≡ only when there is something to get or pull, and a ≡ with nothing to offer does not open (Q5). |
| T20 | — | `sdl_app.cpp` | fixed 5 | The hand-menu caption words are defined in `sdl_app.cpp`; they belong with the verb letters in `src/input`. |
| T21 | Examine on tablet | `input/touch_overlay.cpp` `layout_tablet` | fixed 6 | On the tablet the buttons float over the game, so the top ones (A, G, P) cover the EXAMINE listing's first lines. See Q4. |
| T22 | Main (phone) | `layout_phone` | fixed 6 | At 1248x576 the ⇥ ↷ column reaches 11 px into the game, as the Main board's own 170 vs 162 does. Check whether that is wanted or should be scaled down to fit the margin. |
| T23 | Incant | `sdl_app.cpp` | fixed 5 | The text box and the ⌫ ↵ ✕ SPC labels use SDL's 8 px debug font, which is small next to the keys. |
| T24 | Incant | `sdl_app.cpp` | fixed 5 | The Incant board keeps both A buttons visible over the keyboard; the keyboard currently hides every button. |
| T25 | Issue #34 | system menu | todo | Add a delete action for the system menu's save slots. |
| T17 | — | system menu | todo | The Video/Controls menu entries (goal 5 of `phase-8-goals.md`). Out of scope for this loop; tracked here. |

## Open questions for Steve

(none open)

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
- 2026-09-28, answers from Steve:
  - Q1: Get lists floor items even in the dark, as EXAMINE does. The current
    behaviour stands.
  - Q2: tap outside closes a menu, with no ✕ button. The current behaviour
    stands.
  - Q3 (S1): state transitions look better and there are no current bugs. S1 is
    closed.
- Run 3, 2026-09-28: S5, S6, T13, T14.
  - Both layouts now come from one board arrangement (`layout_boards`):
    - Main board coordinates at 844x390.
    - Phone scaled by height.
    - Tablet at board size with the bottom clusters above the status band.
    - Movement is ⇤ ↑ ⇥ / ↶ ↻ ↷ / ↓ with line-drawn icons; ≡ and ⌨ are also
      icons; the system menu is a pause sign.
    - E and L are one toggle slot, chosen by `OverlayState::examining`.
  - `dod --shots=<script>` now saves scripted screenshots through the offscreen
    SDL driver. The loop can see the screen from now on.
  - Screens checked: tablet main, picker, examine; phone main.
  - New rows: T21 and T22. New questions: Q4 and Q5.
  - `test_phone_matches_main_board` fails when the layout is reverted.
- Run 4, 2026-09-28: T11, T12, S7.
  - Added an on-screen keyboard (`keyboard_layout`, `press_keyboard_key` in
    `src/input`; `OverlayBridge::press_key`), laid out at the Incant board's
    coordinates.
  - ⌨ opens it for a free command line, with an added SPC key that the Incant board does not
    draws.
  - Hand-menu I opens it for INCANT and types `I <word>`.
  - The text box echoes the line being typed; ✕ cancels.
  - Screens checked: keyboard on tablet and on phone.
  - Tests: `test_keyboard_matches_incant_board` and
    `test_keyboard_types_a_line`. The incant test fails when the `I ` prefix is
    removed.
  - New rows: T23 and T24. Q4 and Q5 still open.
- Run 5, 2026-09-28: T15, T16, T20, T23, T24.
  - Keyboard labels and the text box are drawn 1.5–2× larger (T23).
  - Both A buttons stay drawn and live over the keyboard, through
    `OverlayBridge::handle_attack_tap` (T24).
  - The hand-menu captions come from `dag::input::hand_verb_caption` (T20).
  - `handle_tap` passes the real `OverlayState` (T15).
  - `touch-input.md` §4–5 got dated addenda: pickers decided, and all 15 verbs
    are now implemented in the core (T16).
  - Screen checked: keyboard on tablet with typed text.
  - The new bridge check fails without `handle_attack_tap`.
  - Q4 and Q5 still open.
- 2026-09-28, answers from Steve:
  - Q4: hide the top-corner buttons while examining, but only where they cover
    the picture (not on the phone).
  - Q5: offer G only with something on the floor, and P only with something in
    the pack.
  - T22: shrink the controls to fit the margin.
- Run 6, 2026-09-28: S8, S9, T19, T21, T22.
  - Each hand shows only A over ≡.
  - An empty hand's ≡ offers G and P according to floor and pack; choosing one
    opens the named picker beside the same ≡ (`hand_menu_opens`).
  - A full hand's ≡ offers S D U R, plus I with a ring.
  - The ⌨ button and the free-typing keyboard are removed; the keyboard is
    INCANT's only.
  - On the phone, controls shrink so the move cluster fits the margin.
  - While examining, top buttons over the picture are left out: on the tablet,
    not on the phone.
  - Screens checked: tablet ≡ menu (P only), tablet examine, phone examine.
  - New questions: Q6 and Q7.
- 2026-09-28, answers from Steve:
  - Q6: disregard; `--layout=tablet` already exists.
  - Q7: touch saving goes through the system menu's Save/Load. ZSAVE and ZLOAD
    stay original and keyboard-only on touch.
  - Issue #34 records the options, including a possible later "Tape" screen.
    Delete-slot is added to the backlog as T25.
