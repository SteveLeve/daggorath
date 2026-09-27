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
