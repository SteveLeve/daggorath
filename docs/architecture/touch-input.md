# Touch input: command coverage

Living document for Phase 8. This section (8.0) is the interaction inventory:
every command form the parser accepts, its typed keystrokes, and the touch
control that produces them, per the agreed design
([`docs/design/touch-controls/README.md`](../design/touch-controls/README.md)).
Later workstreams add the adapter, shell, render-style and layout-evaluation
sections named in [`docs/planning/phase-8-plan.md`](../planning/phase-8-plan.md).

Sources: [`docs/specification/commands-and-parser.md`](../specification/commands-and-parser.md)
(token tables, §1; dispatch, §3), [`docs/specification/combat-and-items.md`](../specification/combat-and-items.md)
(`PATTK`, `PGET`/`PDROP`/`PSTOW`/`PPULL`, `PUSE`, `PINCAN`), and the decoded
token fixtures `docs/archaeology/phase-0b/fixtures/tokens.json` and
`parser-prefixes.json`. Labels follow those documents: **[SRC]** is the
listing, **[INF]** is this project's inference, unmarked rows restate a
**[SRC]** row from those documents without new claims.

## 1. Verb inventory (`CMDTAB`, 15 entries) — [SRC]

Every verb the parser accepts, its shortest unique prefix (`parser-prefixes.json`),
what the core currently dispatches (`commands-and-parser.md` §3), and the touch
control that types it.

| Verb | Prefix | Core dispatch | Touch control | Keystrokes produced |
|---|---|---|---|---|
| `MOVE` | `M` | dispatched (`PMOVE`/`PSTEP`/`PMOV90`) | bottom-left `↑ ↓ ⇤ ⇥` | `M` · `M B` · `M L` · `M R` |
| `TURN` | `T` | dispatched (`PTURN`/`PREVU`) | bottom-left `↶ ↷ ↻` | `T L` · `T R` · `T A` |
| `LOOK` | `L` | dispatched (`PLOOK`, selects viewer mode) | bottom-right `L` | `L` |
| `ATTACK` | `A` | `UNIMPLEMENTED` (Phase 3 spec exists, not yet wired to `CMDTAB`) | top-corner `A` (left/right) | `A L` · `A R` |
| `GET` | `G` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `G`, empty hand only → floor picker | `G L <obj>` · `G R <obj>` |
| `PULL` | `P` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `P`, empty hand only → pack picker | `P L <obj>` · `P R <obj>` |
| `STOW` | `S` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only | `S L` · `S R` |
| `DROP` | `D` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only | `D L` · `D R` |
| `USE` | `U` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only | `U L` · `U R` |
| `REVEAL` | `R` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only | `R L` · `R R` |
| `INCANT` | `I` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only → in-game keyboard | `I ` then the typed adjective, e.g. `I VULCAN` |
| `EXAMINE` | `E` | `UNIMPLEMENTED` (Phase 4 spec exists) | bottom-right `E` | `E` |
| `CLIMB` | `C` | `UNIMPLEMENTED` (unresolved effect, §5) | bottom-right `C` → choose `U`/`D`, offered on ladder or hole | `C U` · `C D` |
| `ZLOAD` | `ZL` | `UNIMPLEMENTED` (Phase 4/5 spec exists) | bottom-right `⌨` only (free command line) | `ZLOAD` (or any unambiguous prefix `ZL...`) |
| `ZSAVE` | `ZS` | `UNIMPLEMENTED` (Phase 4/5 spec exists) | bottom-right `⌨` only (free command line) | `ZSAVE` (or `ZS...`) |

Every verb has a touch path. `ZLOAD`/`ZSAVE` are deliberately keyboard-only
(design doc §"Interaction rules"): they need a filename-free confirmation
flow the mockups do not model, and the typed line is always available as the
fallback for every command, including these two.

A bare `Z` matches both `ZLOAD` and `ZSAVE` and is rejected by the parser
(ambiguous prefix, §1); no control types a bare `Z`.

## 2. Direction and adjective tokens (`DIRTAB`, `ADJTAB`) — [SRC]

`DIRTAB`: `LEFT`, `RIGHT`, `BACK`, `AROUND`, `UP`, `DOWN` (prefixes `L`, `R`,
`B`, `A`, `U`, `D`; all shortest-unique at one letter). Which directions a verb
accepts is per-verb, not per-table:

| Verb | Accepted directions | Rejected (`???`) | Touch source |
|---|---|---|---|
| `MOVE` | none (forward), `BACK`, `LEFT`, `RIGHT` | `AROUND`, `UP`, `DOWN`, unrecognised | `↑`/`⇤`/`⇥` type `M`/`M L`/`M R`; `↓` types `M B` |
| `TURN` | `LEFT`, `RIGHT`, `AROUND` | `BACK`, `UP`, `DOWN`, missing | `↶`/`↷`/`↻` type `T L`/`T R`/`T A` |
| `ATTACK` | `LEFT`, `RIGHT` (hand, via `PARHND`) | anything else | `A` buttons type `A L`/`A R` |
| `CLIMB` | unresolved (§5); design assumes `UP`/`DOWN` from the ladder/hole context | — **[INF]** not yet checked against a `CLIMB` listing routine | `C` button offers `C U`/`C D` |

`ADJTAB` (25 entries) covers ring adjectives (`VULCAN`, `RIME`, `JOULE`,
`SUPREME`) and item adjectives (`ELVISH`, `MITHRIL`, `SEER`, `THEWS`, `VISION`,
`ABYE`, `HALE`, `SOLAR`, `BRONZE`, `IRON`, `LUNAR`, `PINE`, `LEATHER`,
`WOODEN`) plus the four revealed ring names and `GOLD`, `EMPTY`, `DEAD`. The
`INCANT` in-game keyboard (design doc) accepts any of these; the parser
rejects anything that is not a full adjective (`PINCAN` "requires the full
adjective", `combat-and-items.md`).

`GENTAB` (6 entries): `FLASK`, `RING`, `SCROLL`, `SHIELD`, `SWORD`, `TORCH` —
the generic names `GET`/`PULL` pickers must offer, per object class.

## 3. Line editing and the 32-byte buffer — [SRC] / [INF]

**[SRC]** `commands-and-parser.md` §4: the line buffer is 32 bytes; the 32nd
character dispatches without Return; the keyboard buffer is also 32 bytes with
no overflow check, and a burst of exactly 32 leaves head equal to tail (reads
as empty — the buffer-overrun quirk).

**[INF]** deviation D-17 (planned) requires every touch line to be at most 31
characters including the terminating CR, so a touch gesture cannot by itself
produce the 32-character overrun; that stays reachable only by typing. The
longest touch-composed line here is `INCANT` with a 7-letter adjective:
`I SUPREME\r` = 10 bytes, well under the limit. The overrun is preserved as a
typing-only path (`⌨`), not removed as a possibility — 8.1 must show it is
still reachable by typing through the same keyboard buffer the touch adapters
feed.

## 4. Picker contents — [INF], to close in implementation

Per the design doc's decision: floor and pack pickers show only what the
player can see. **[INF], not yet checked against the listing (open item
carried from the plan):**

- Floor picker: objects drawn on the player's current cell, none while dark.
  The exact "objects on this cell" read needs a core accessor (`combat-and-items.md`
  `PGET`/`PDROP` describe the floor-object list but not a query API); if
  missing, add it as an isolated `src/core` read-only-accessor commit per the
  goal's rule, with its own boundary check.
- Pack picker: names as the `EXAMINE` display gives them (`combat-and-items.md`
  `USE`/`EXAMINE`); `EXAMINE`'s display format is otherwise unresolved in the
  specification (`commands-and-parser.md` §5 lists it as not yet specified),
  so the exact name strings are an 8.1/8.4 implementation detail, not decided
  here.

**Addendum (2026-09-28, touch-overlay run 5):** The pickers are now built,
with Steve deciding the one open question.
- The floor picker lists the objects EXAMINE lists on the player's cell,
  whether or not there is light, as EXAMINE itself does (Steve,
  2026-09-28, answering Q1 in `docs/planning/touch-overlay-log.md`).
- The pack picker lists the backpack names as EXAMINE gives them.
- No new core accessor was needed. `dag::platform::overlay_state_from` reads
  the existing `examine_snapshot_from` (presentation) and `vfind` (core; PCLIMB.ASM:13 `JSR VFIND`,
  [SRC]).

## 5. Commands with unresolved effect

`ATTACK`, `CLIMB`, `DROP`, `EXAMINE`, `GET`, `INCANT`, `PULL`, `REVEAL`,
`STOW`, `USE`, `ZLOAD`, `ZSAVE` are `UNIMPLEMENTED` in the core today
(`commands-and-parser.md` §3, §5). The coverage table above is a contract for
touch to reach the same keystrokes a typist would use; it does not require the
core to implement these verbs, and does not approximate them. Phase 8 adds no
gameplay behaviour (`docs/prompts/phase-8-touch-input.md`: "do not build...
rule changes").

**Addendum (2026-09-28):** this section and the "core status" column of §1
are out of date. `Game::dispatch_line` (src/core/game.cpp) now handles all 15
verbs:
- MOVE, TURN and LOOK;
- ATTACK, CLIMB, DROP and EXAMINE;
- GET, INCANT, PULL and REVEAL;
- STOW, USE, ZLOAD and ZSAVE.

The contract is unchanged: touch types the same keystrokes a typist would.

**Addendum (2026-09-28, touch-overlay run 6):** Steve's decisions of
2026-09-28 supersede parts of §1 and §3. The details are in
`docs/planning/touch-overlay-log.md`.
- **GET and PULL** are no longer separate buttons under `A`. They are
  entries in the empty hand's `≡` menu:
  - `G` appears only when something is on the floor;
  - `P` appears only when the pack holds something.
- **The `⌨` free-command-line button is removed.** The on-screen keyboard
  opens only for INCANT, from `≡` → `I`.
- **The typed-line fallback in §1 and §3 now exists only on a physical
  keyboard.** On touch that affects two routes:
  - §3's INCANT-overrun path;
  - ZLOAD and ZSAVE (rows 39–40).
- **§6's gate for ZLOAD and ZSAVE:** reopened by the removal above, then
  closed by Steve's answer to Q7 (2026-09-28). They are keyboard-only by
  design. On touch, saving goes through the system menu's Save/Load, the
  shell's slots plus the hidden resume slot. Issue #34 carries the optional
  later "Tape" screen for the original commands.

## 6. Coverage gate

Every one of the 15 `CMDTAB` verbs has at least one touch path to its
keystrokes, or is explicitly keyboard-only with a stated reason (`ZLOAD`,
`ZSAVE`, §1). This closes the 8.0 completion gate ("coverage table: every
command form reachable by touch") for the command language as currently
specified. Two follow-ups are carried into later workstreams, not reopened
here:

- §4's floor/pack visibility rules, checked against the listing when `GET`/
  `PULL`/`EXAMINE` are implemented (outside Phase 8's scope per the prompt).
- §2's `CLIMB` direction handling, checked against the listing when `CLIMB` is
  implemented.

## 7. Adapter API (8.1)

`src/input/gesture.{hpp,cpp}` (`dag::input`). `GestureLine` composes a touch
control's command text (§1) into `dag::KeyEvent`s stamped on one shared
jiffy, per deviation D-17 (`docs/specification/clock-and-scheduler.md` §13):
unlike typed input's one-key-per-jiffy pacing, a finished gesture delivers
its whole line at once. Enforced at `kMaxGestureLine` = 31 characters
including CR, so a gesture can never itself produce the 32-byte keyboard
buffer overrun (typing-only, `t5-keyboard-overrun`). Per-gesture fixtures:
`docs/archaeology/phase-8/fixtures/gesture-lines.txt`, checked by
`tests/input/gesture_tests.cpp`.

## 8. Shell (8.2, ADR-0009)

`src/shell/shell.{hpp,cpp}` (`dag::shell::Shell`), headless (links only
`daggorath::core`, no SDL). What it does to a running `Game` — nothing else,
per ADR-0009 §2:

- **Pause/resume** (`pause()`/`resume()`/`tick()`): pausing withholds jiffy
  delivery entirely (`tick()` becomes a no-op); resuming continues from the
  same clock value, so no jiffy is owed for paused wall time (D-16).
- **Five save/load slots plus one hidden slot** (`slots()`, `save_to_slot`,
  `load_from_slot`, `write_hidden_slot`/`restore_hidden_slot`), each a
  `Game::snapshot()`/`restore_snapshot()` suspend snapshot, auto-named from
  dungeon level and time played (`Shell::auto_name`, read-only core
  accessors only). The shell validates a slot's snapshot magic/version
  itself before calling `restore_snapshot`, which otherwise aborts on a bad
  one (ADR-0009 §5).
- **Confirmations** (`ConfirmKind`): an occupied-slot overwrite, Restart, and
  Quit all set a pending confirmation instead of acting immediately;
  Restart/Quit are handed back to the caller to perform, since constructing
  a new `Game` or exiting the process is outside what ADR-0009 §2 lets the
  shell do itself.
- **Trace markers**: `PAUSE`/`RESUME` lines are recorded in `Shell::trace()`,
  separate from `Game::trace()` — they are shell lines, not `CoreEvent`s
  (ADR-0004 rule 1), so the core trace stays diffable with ROM traces
  without filtering.

**Pause-invariance test** (ADR-0009 §4, the 8.2 gate):
`tests/shell/shell_tests.cpp` `test_pause_invariance` runs the same scripted
keystrokes twice — once with no shell involved, once wrapped in a `Shell`
that pauses partway through and resumes later — and asserts `Game::trace()`
is byte-for-byte identical between the two. `test_save_and_load_round_trip`
and related tests cover slot save/load, overwrite confirmation, and the
hidden slot.

The SDL menu UI (Esc key, the on-screen button, backgrounding hook) is
deferred to 8.4, which wires this headless shell into `src/platform`;
ADR-0009 is accepted on that basis (see its Resolution).

**Addendum (2026-09-27, 8.6.2):** Esc and the `SystemMenu` overlay button
now toggle `pause()`/`resume()` in `src/platform/sdl_app.cpp`. The unpaused
window still runs and renders correctly (screenshot-confirmed); actually
entering the paused state was not, for lack of click/keypress-automation
tooling in this sandbox (`docs/archaeology/phase-8/reconciliation.md`'s
addendum has the full account). Still
not built: the Save/Load/Restart/Quit menu surface (Resume-by-toggle is the
only way back from paused) and the OS-backgrounding hook — both real,
recorded gaps, not design decisions.

## 9. Crisp render style (8.3, ADR-0010)

`src/presentation/crisp.{hpp,cpp}`, headless (no SDL). `build_crisp_frame`
consumes `RenderState::segments` — the same logical draw list `pixel`'s
`raster.cpp` rasterises (ADR-0010 §2), not a second geometry source. A
fully-lit segment (`fade == 0`) becomes one continuous line; a dim segment
becomes the exact dots `draw_segment` would plot (found via the newly shared
`raster.hpp::walk_segment`), matching ADR-0010 §8's dotted dimness. `build_crisp_map`
projects the same `MapSnapshot` `rasterize_map` (`mapper.hpp`) rasterises: a
solid-wall cell becomes one filled square (§7), and the player/object/
creature/vertical-feature marks stay nearest-neighbour bitmap cells (small
bitmaps, not squares) via the newly shared `mark4_rows`.

Segment fixtures, one per Phase 6 golden state:
`docs/archaeology/phase-8/fixtures/crisp-segments.txt`, regenerated and
checked byte-for-byte by the `crisp_segment_fixtures` ctest. The Phase 6/7
golden pixel images are unchanged — confirmed by `viewer_regressions`,
`text_regressions`, and `make verify`'s fixture manifests, all still passing
after the `walk_segment`/`mark4_rows` refactor that let both render styles
share one implementation.

Not built here: device-pixel scaling, line thickness, HiDPI handling, and
smoothing are SDL3 platform concerns this headless module leaves to the
caller (it emits source-256x192 coordinates only) — deferred to 8.4, which
wires `crisp`/`pixel` into `src/platform`. Text stays on the existing bitmap
path (`text.cpp`); ADR-0010 §6's all-vector-text option is not reopened.

## 10. Touch overlay prototype (8.4)

`src/input/touch_overlay.{hpp,cpp}`, headless (no SDL): pure layout, hit
testing, and gesture dispatch, so the mouse-as-touch prototype's logic is
tested without a display. Two layouts evaluated, per
`docs/design/touch-controls/README.md`'s decisions:

- **Phone landscape**: hand controls (`A`, then `G`/`P` or the hand menu) in
  the top corners; movement and turn controls at bottom left; climb (when
  offered)/examine/look/keyboard/system-menu at bottom right.
- **Tablet 4:3**: the same hand-control corners, but movement/turn and the
  climb/examine/look/keyboard/system-menu clusters run as compact 2-column
  grids down the left and right edges instead of a bottom row, so the
  status and command lines stay clear (`bottom_band` = the lower quarter of
  the viewport; `tests/input/touch_overlay_tests.cpp`
  `test_tablet_stays_clear_of_bottom_band` checks this by construction).

`layout_buttons` computes every visible control's hit rectangle for a
viewport and hand state (`OverlayState`: which hand is empty, whether a
ladder/hole offers Climb); `hit_test` finds which rectangle a point falls in
— the same function whether that point comes from a mouse click or a touch
tap, which is what "mouse-as-touch" means here. `resolve_tap` turns a button
press into either a finished command line (fed to `GestureLine`, 8.1) or a
`PendingKind` (a floor/pack picker, the holding-hand `≡` menu, the climb
`U`/`D` confirmation, or the incant keyboard) that `resolve_picker_choice`
finishes once the player picks an object, a verb letter, or a direction.

**Recorded obstacle (2026-09-27):** this sandbox has no SDL3 available and
no path to build it from source in this session (not in the package
manager; building from source needs the source tree and time this session
does not have). The actual on-screen rendering of these buttons in
`src/platform/sdl_app.cpp`, the manual landscape/tablet evaluation the 8.4
gate calls for, and wiring the shell's system-menu button (ADR-0009) and
`crisp` (ADR-0010) into that window are therefore not built here. This
module is deliberately headless so its layout math and gesture dispatch
could still be built and tested; the SDL platform work is carried forward,
recorded once rather than re-litigated at each PR.

## 11. Replay equivalence (8.5)

`tests/input/replay_equivalence_tests.cpp` closes the Phase 8 prompt's own
requirement: "a replay of a touch session produces the same core trace as
its keystroke transcript." Two checks:

- **Against a committed, provenance-tracked trace.** `gesture_turn_right()`
  ("T R") is exactly the command the committed
  `docs/archaeology/phase-0b/traces/t3-burst-one-jiffy` fixture types as a
  hand-authored, same-jiffy burst. Driving a fresh `Game` from
  `GestureLine(gesture_turn_right(), 5)`'s own `KeyEvent`s for 200 jiffies
  produces a trace byte-for-byte identical to that fixture (header line, the
  full `TraceEvent` stream, and the `# final` player-state footer dcli
  writes). The touch adapter and the scripted burst are, and must stay, the
  same input to the core.
- **Touch burst vs. typed pacing.** `gesture_move_forward()` ("M") delivered
  as a touch burst at jiffy 5, and the same two bytes typed one jiffy apart
  finishing (`CR`) on that same jiffy, produce identical traces. `PLAYER`
  drains whatever the keyboard buffer holds when it next polls (`task_player`,
  `game.cpp`), not when each byte individually arrived, so a line delivered
  in one burst and the same line typed but complete by the same jiffy
  dispatch identically. This is the case D-17 gives up (queue saturation,
  the 32-byte overrun, mid-burst dispatch) staying reachable only when the
  two delivery patterns genuinely diverge — a longer or interleaved
  sequence than this test's two short commands — which typed input alone
  still exercises (`t5-keyboard-overrun`).

Together with the pause-invariance test (§8) and the crisp segment fixtures
(§9), this closes the Phase 8 completion gate's test-output requirements.
Section 6 already closes the coverage-table requirement. The remaining gate
items — `make all` output and the README phase table — are recorded in
`docs/archaeology/phase-8/reconciliation.md`.

## Layouts evaluated and the chosen default (8.4, evidence)

Two layouts were built and tested (§10): phone landscape (bottom-corner
controls) and tablet 4:3 (edge-hugging 2-column grids, clear of the status
band). Both were headless prototypes as of 2026-09-27.

**Addendum (2026-09-27, workstream 8.6):** SDL3 became available in the
build environment; `docs/archaeology/phase-8/reconciliation.md`'s addendum
has the full account. `Tablet4x3` is now wired into `src/platform/sdl_app.cpp`
and evaluated on screen — a real screenshot of the running `dod` window
(kept outside the tree at
`captures/phase-8-sdl-wiring/tablet4x3-buttons-2026-09-27.png`, gitignored)
shows all 15 buttons at `layout_buttons()`'s computed positions, legible labels,
chrome unaffected. It was the natural choice: the fixed 768x576 (4:3) desktop
window matches that layout's own assumption without any letterboxing.
**Addendum (2026-09-27, workstream 8.6.5):** `PhoneLandscape` is now also
evaluated on screen. `sdl_app.cpp`'s `--layout=phone` opens a 1248x576
(19.5:9) window with the 768x576 game view centered and letterboxed; a
screenshot
(`captures/phase-8-sdl-wiring/phonelandscape-buttons-2026-09-27.png`,
gitignored) confirms the corner/edge buttons land in the black side
margins, chrome centered and unaffected, matching the design doc's
"controls sit in the black side margins" intent. `--layout=tablet` (the
default) is unchanged and still screenshot-confirmed above. With both now
genuinely evaluated, neither on-screen check gives a reason to override the
design doc's own decision: device form factor still selects the layout
(phone landscape for phones, tablet 4:3 for tablets), not a single
this-project-wide default. The 8.4 gate's manual-evaluation requirement is
closed by this addendum.
