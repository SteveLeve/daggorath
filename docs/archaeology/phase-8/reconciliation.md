# Phase 8 reconciliation

Written 2026-09-27. Reconciles the six workstreams in
[`docs/planning/phase-8-plan.md`](../../planning/phase-8-plan.md) against
what actually landed, per [`docs/prompts/phase-8-touch-input.md`](../../prompts/phase-8-touch-input.md)'s
completion gate. This is Phase 8's own record, in the same spirit as the
earlier phases' reconciliation notes: what is settled, and what the source
contradicts or this environment could not build.

## Workstreams, as delivered

**Merge status (2026-09-27, when this note was written): only #25 (8.0) is
merged to `main`.** #26-29 (8.1-8.4) are open pull requests, each reviewed
by `boundary-checker`/`evidence-auditor` and passing `make all` on its own
branch, not yet merged. This branch (8.5) merges #26-29 in locally so its
own tests and this document can exercise and describe the whole phase
together; that is a local integration for writing and testing this note,
not a claim that `main` already contains them. "Complete" below means every
workstream's work exists, is tested, and is queued to merge — not that
`main` has absorbed it yet. Whoever merges these five PRs should merge them
in order (8.0 already is; then #26, #27, #28, #29, then this one), since
each was branched from the one before it and carries its content forward.

| # | PR | Merged to `main`? | What landed | What did not |
|---|---|---|---|---|
| 8.0 | #25 | **yes** | Coverage table: every `CMDTAB` verb has a touch path or a stated keyboard-only reason (`docs/architecture/touch-input.md` §1-6) | — |
| 8.1 | #26 | no, open | `src/input/gesture.{hpp,cpp}`: whole-line, one-jiffy gesture adapters (D-17 implemented); per-gesture fixtures | — |
| 8.2 | #27 | no, open | `src/shell/shell.{hpp,cpp}`: pause/resume (D-16 implemented), five slots + hidden slot, confirmations, PAUSE/RESUME trace markers, pause-invariance test; ADR-0009 Resolution | Video/Controls menu entries, option-setting, keystroke delivery through the shell, and the Esc-key/on-screen-button SDL wiring |
| 8.3 | #28 | no, open | `src/presentation/crisp.{hpp,cpp}`: segment/dot/map geometry from the same draw list `pixel` rasterises; per-golden-state segment fixtures; golden images unchanged; ADR-0010 Resolution | Device-pixel scaling, line thickness, HiDPI, smoothing, text-as-geometry (SDL3 platform concerns; text explicitly not reopened) |
| 8.4 | #29 | no, open | `src/input/touch_overlay.{hpp,cpp}`: headless layout, hit-testing and gesture dispatch for phone landscape and tablet 4:3, mouse-as-touch | The actual SDL rendering, the manual landscape/tablet evaluation, and wiring the shell/`crisp` into the desktop window |
| 8.5 | (to be opened) | no | `tests/input/replay_equivalence_tests.cpp`: a touch session's trace matches the committed scripted-burst fixture byte for byte, and a touch burst matches typed pacing finishing on the same jiffy; this reconciliation note; `docs/architecture/touch-input.md` finished through §11 | A chosen default between the two layouts (needs the manual evaluation 8.4 could not do here) |

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

Every artifact the gate names exists and passes, checked on this branch
(#26-29 merged locally into it, per the merge-status note above). This
checklist is not a claim that `main` already has them — see that note —
only that the work itself is done and ready to merge.

- [x] Coverage table (every command form reachable by touch) — §1-6.
- [x] Replay-equivalence test output — `replay_equivalence_tests`, 7 checks passing.
- [x] Pause-invariance test output — `shell_tests::test_pause_invariance`, part of 29 checks passing.
- [x] Crisp segment fixtures — `docs/archaeology/phase-8/fixtures/crisp-segments.txt`, checked by `crisp_segment_fixtures`.
- [x] `make all` output — 16/16 tests passing (17 with the disabled playthrough), fixture manifests clean, golden images unchanged.
- [x] README phase table showing Phase 8 complete.
- [x] This reconciliation note.
- [ ] PRs #26, #27, #28, #29, and this workstream's own PR merged to `main`, in that order.
