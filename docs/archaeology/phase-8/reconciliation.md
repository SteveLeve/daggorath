# Phase 8 reconciliation

Written 2026-09-27. Reconciles the six workstreams in
[`docs/planning/phase-8-plan.md`](../../planning/phase-8-plan.md) against
what actually landed, per [`docs/prompts/phase-8-touch-input.md`](../../prompts/phase-8-touch-input.md)'s
completion gate. This is Phase 8's own record, in the same spirit as the
earlier phases' reconciliation notes: what is settled, and what the source
contradicts or this environment could not build.

## Workstreams, as delivered

**Merge status (updated 2026-09-27): #25-29 (8.0-8.4) are all merged to
`main`**, in order, each rebased onto the previous merge and re-verified
(`make all` green, no fixture drift) before merging. This PR (8.5) is the
last workstream, opened once #26-29 landed. Between when this section was
first written (only #25 merged) and now, a bug-fix branch
(`refinement/playthrough-and-discovery`, PR #23) merged to `main` ahead of
this work; each Phase 8 branch was rebased onto that fix (and onto each
prior Phase 8 merge) before merging, so `main`'s history is linear and every
merge was re-verified independently, not just carried forward assuming it
still worked.

| # | PR | Merged to `main`? | What landed | What did not |
|---|---|---|---|---|
| 8.0 | #25 | **yes** | Coverage table: every `CMDTAB` verb has a touch path or a stated keyboard-only reason (`docs/architecture/touch-input.md` §1-6) | — |
| 8.1 | #26 | **yes** | `src/input/gesture.{hpp,cpp}`: whole-line, one-jiffy gesture adapters (D-17 implemented); per-gesture fixtures | — |
| 8.2 | #27 | **yes** | `src/shell/shell.{hpp,cpp}`: pause/resume (D-16 implemented), five slots + hidden slot, confirmations, PAUSE/RESUME trace markers, pause-invariance test; ADR-0009 Resolution | Video/Controls menu entries, option-setting, keystroke delivery through the shell, and the Esc-key/on-screen-button SDL wiring |
| 8.3 | #28 | **yes** | `src/presentation/crisp.{hpp,cpp}`: segment/dot/map geometry from the same draw list `pixel` rasterises; per-golden-state segment fixtures; golden images unchanged; ADR-0010 Resolution | Device-pixel scaling, line thickness, HiDPI, smoothing, text-as-geometry (SDL3 platform concerns; text explicitly not reopened) |
| 8.4 | #29 | **yes** | `src/input/touch_overlay.{hpp,cpp}`: headless layout, hit-testing and gesture dispatch for phone landscape and tablet 4:3, mouse-as-touch | The actual SDL rendering, the manual landscape/tablet evaluation, and wiring the shell/`crisp` into the desktop window |
| 8.5 | #30 | pending this PR | `tests/input/replay_equivalence_tests.cpp`: a touch session's trace matches the committed scripted-burst fixture byte for byte, and a touch burst matches typed pacing finishing on the same jiffy; this reconciliation note; `docs/architecture/touch-input.md` finished through §11 | A chosen default between the two layouts (needs the manual evaluation 8.4 could not do here) |

## Recorded obstacle: no SDL3 in this environment

Every workstream that would touch `src/platform`/`src/app`'s SDL window —
the shell's on-screen menu button and Esc key, `crisp`'s actual device
rendering, and the touch overlay's real button graphics — could not be
built or tested here. This sandbox has no SDL3 package and no path to build
it from source within a session (the source tree and the time to configure
and build it are both outside this session's scope). `tools/check-sdl3-deps.sh`
and `src/platform/CMakeLists.txt` already handle this gracefully (the `dod`
desktop target is skipped, not failed, when SDL3 is absent), and Phase 7's
own desktop work was built and tested elsewhere, on a machine with SDL3.

Recorded once, per CLAUDE.md's rule against re-litigating an obstacle every
turn: `docs/architecture/touch-input.md` §10 and the `touch_overlay.hpp`
header comment. Every workstream instead built the layer beneath the SDL
boundary headlessly (module-boundaries.md already names this as `src/shell`'s
eventual target once it has "non-SDL logic worth testing headlessly" — true
of 8.2, 8.3 and 8.4 alike) and proved it with tests that run in this
environment. The SDL wiring itself — three ADRs/sections all point to it —
is real, un-done follow-up work, not a gap this reconciliation is hiding.

## Deviations closed

- **D-16** (shell pause): implemented, `src/shell/shell.{hpp,cpp}`.
  Pause-invariance test: `tests/shell/shell_tests.cpp::test_pause_invariance`.
- **D-17** (touch gestures, one-jiffy burst): implemented,
  `src/input/gesture.{hpp,cpp}`. Replay-equivalence test:
  `tests/input/replay_equivalence_tests.cpp`.

## ADRs settled

- **ADR-0009** (layering, shell, parallel work): accepted, scoped to what
  8.2 built (dated Resolution in the ADR itself). ADR-0007 rule 5 narrowed
  accordingly.
- **ADR-0010** (render styles): accepted, scoped to what 8.3 built (dated
  Resolution in the ADR itself).

## Genuinely new question raised

None. The design decisions in `docs/planning/phase-8-plan.md` and
`docs/design/touch-controls/README.md` were followed as settled; no PR
reopened them. The one open item — which layout is the default — is not a
new question but the 8.4 gate's own manual-evaluation requirement, which
this environment cannot perform.

## Completion gate checklist

Every artifact the gate names exists and passes, re-verified after each
merge onto `main`'s current tip (rebuilt and re-tested, not just carried
forward).

- [x] Coverage table (every command form reachable by touch) — §1-6.
- [x] Replay-equivalence test output — `replay_equivalence_tests`, 7 checks passing.
- [x] Pause-invariance test output — `shell_tests::test_pause_invariance`, part of 29 checks passing.
- [x] Crisp segment fixtures — `docs/archaeology/phase-8/fixtures/crisp-segments.txt`, checked by `crisp_segment_fixtures`.
- [x] `make all` output — 16/16 tests passing (17 with the disabled playthrough), fixture manifests clean, golden images unchanged.
- [x] README phase table showing Phase 8 complete.
- [x] This reconciliation note.
- [x] PRs #25, #26, #27, #28, #29 merged to `main`, in that order (each rebased onto the previous merge and `main`'s own `refinement/playthrough-and-discovery` fix, PR #23, and re-verified before merging). #30 (this PR) is the last of the six.

## Addendum (2026-09-27): the recorded obstacle is reopened

The "no SDL3 in this environment" obstacle recorded above no longer holds.
`pkg-config --modversion sdl3` now reports `3.2.31`, `SDL3_DIR` is cached in
`build/CMakeCache.txt`, and `src/platform/CMakeLists.txt`'s `find_package(SDL3
QUIET)` succeeds — the `dod` desktop target builds and runs in this sandbox
today. This is left as-is above (a correct record of what was true on
2026-09-27 at the time this reconciliation was written); this addendum
records what changed and what workstream 8.6 (branch
`phase-8/sdl-platform-wiring`) did about it, per this project's rule against
regenerating a record instead of appending to it.

Before 8.6, `src/platform/sdl_app.cpp` had **no** touch, shell, or `crisp`
wiring at all, confirmed by grep — exactly the Phase 7 desktop window,
unchanged. So none of §8-10's on-screen work had actually been built or
evaluated; it existed only as the headless modules and their tests, plus the
static HTML mockups under `docs/design/touch-controls/mockups/`.

**8.6.1 (touch overlay rendering + input).** Added
`src/platform/overlay_bridge.{hpp,cpp}` (`dag::platform::OverlayBridge`):
links only `daggorath::input`, no SDL, so its tap/picker state machine is
headless-testable (`tests/platform/overlay_bridge_tests.cpp`, 23 checks).
Wired into `sdl_app.cpp`: mouse-down events hit-test the `Tablet4x3` layout
(the fixed 768x576 desktop window matches that layout's 4:3 assumption
natively) and press the resolved command line's keystrokes via `Game::press`
— not `GestureLine`'s same-jiffy scripted burst, which needs
`Game::load_script` and would risk dropping a live game's not-yet-consumed
keystrokes; see the header comment for the full reasoning. Buttons render as
outlined rects with single-letter placeholder labels (real icon art is a
follow-up polish item, not a correctness gap): confirmed by a real screenshot
of the running `build/src/platform/dod` window (`import -window`, this
sandbox's `DISPLAY=:0`), all 15 buttons at the positions `layout_buttons()`
computes, labels legible, chrome (status/command lines) unaffected. Kept
outside the tree at `captures/phase-8-sdl-wiring/tablet4x3-buttons-2026-09-27.png`
(gitignored, per this project's ROM-capture convention) as the record of
what was actually looked at, rather than an unverifiable claim.

**8.6.2 (shell system-menu wiring).** The running `Game` is now wrapped in a
`dag::shell::Shell`; the per-frame `advance_jiffies` call became
`shell->tick(steps)` (a no-op while paused, D-16 — the existing
`shell_tests.cpp::test_pause_invariance` already proves this substitution
changes nothing about an unpaused run's trace, so no new core-level test was
added). Esc (desktop) and the `SystemMenu` overlay button both toggle
`pause()`/`resume()`; while paused, gameplay input is withheld and a minimal
pause banner plus the `SystemMenu` button are coded to render. **Not
screenshot-confirmed**: this sandbox has no `xdotool`/`ydotool`/`wtype`, nor
`XTest.h` headers to build one (checked; not present), so no click or
keypress could be synthesized against the real window to actually enter the
paused state and capture it — unlike 8.6.1's rendering, which needed no
input, only a running window. What *is* confirmed for this half: the
substitution of `shell->tick()` for `game.advance_jiffies()` changes nothing
about an unpaused run (the existing pause-invariance test, above), and the
code path compiles and the window still runs normally unpaused (screenshot
`captures/phase-8-sdl-wiring/tablet4x3-shell-wired-2026-09-27.png`). Whether
the paused banner actually appears and the viewport actually freezes is
untested beyond code review — a real device, or a session with
computer-use/desktop-control tooling, closes this gap. **Not built:** the
Save/Load/Restart/Quit menu surface
ADR-0009 §6 describes — Resume-by-toggle is the only way back from the
paused state in this pass. This is a real, recorded gap, not a design
decision; `Shell` already has everything `slots()`/`save_to_slot`/
`load_from_slot`/`request_restart`/`request_quit` needs, only the on-screen
list is missing.

**8.6.3 (crisp render style toggle): not attempted this session** — flagged
as a time-boxed stretch goal in the plan and deferred rather than half-wired.

**8.6.4 (manual on-screen evaluation): partially closed.** `Tablet4x3` is
now genuinely evaluated on screen (screenshots above). `PhoneLandscape` is
not — this fixed-size window has no letterboxed-aspect simulation mode, and
building one was out of this session's scope. The 8.4 gate's
phone-vs-tablet default decision therefore still stands as the design doc's
interim default (device form factor selects the layout), not a comparison
made here.

**Recorded obstacle: no input-automation tooling in this sandbox.** Verifying
a tap or keypress actually *changes* the running window's behavior (not just
that it renders) needs synthesizing mouse/keyboard events against a real X/
Wayland window. This sandbox has no `xdotool`, `ydotool`, `wtype`, or
`XTest.h` headers to build one, and installing packages or Python modules
system-wide for this was not attempted without asking first. Interactive
correctness is instead proven by `overlay_bridge_tests.cpp` (23 checks,
tap-sequence-to-trace equivalence against typed input) and
`shell_tests.cpp::test_pause_invariance`; on-screen verification in this pass
is limited to *rendering* (screenshots), not *live interaction*. A real
device, or a session with computer-use/desktop-control tooling, closes this
gap.

`make build && ctest --test-dir build` (fixtures/traces/verify unaffected —
no fixture drift): 19/19 active tests passing, `playthrough_power_on_to_winner`
still disabled per Phase 5b, `overlay_bridge_tests` new and passing.
