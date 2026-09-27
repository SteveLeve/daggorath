# ADR-0009 — Layering: the pure port, the shell, and parallel work

**Status:** Proposed, 2026-09-27. Settled when Phase 8 workstream 8.2 lands.

## Context

The core (Phases 0–7) is a conformance-tested recreation of the 1983 program.
The mobile product also needs things the original never had: a system menu
(Esc on desktop, a button on touch) that pauses and offers save, load, restart,
video and control options, as the web port's escape menu does
([port comparison](../planning/port-comparison.md)). ADR-0007 rule 5 put every
pause in enhanced modes, and the Phase 8 prompt banned pausing overlays. Work on
the pure port (Track R captures, Phase 5b, reconciliation fixes) continues while
these layers are built, so the two must not fork.

## Decision

1. **Layers, innermost out.**

   | Layer | Where | May |
   |---|---|---|
   | Core | `src/core` | Everything the original does. Unchanged by this ADR. |
   | Presentation | `src/presentation` | Pure projections of core state (ADR-0004). |
   | Input adapters | `src/input` | Emit timestamped keystrokes only. |
   | **Shell** | `src/platform` for now; its own target (`src/shell`) once it has non-SDL logic worth testing headlessly | System menu, pause, settings, snapshot slots, render style (ADR-0010). |
   | Backends | `src/platform` | SDL3 window, audio, storage; later Android/iOS. |

2. **What the shell may do to a running game** — nothing else:
   - withhold and resume jiffy delivery (pause);
   - take and restore a suspend snapshot through the existing non-gameplay API
     (`Game::snapshot()` / `Game::restore_snapshot()`, ADR-0005 §3);
   - discard the game and construct a new one through the canonical
     construction path (Restart; not a `CMDTAB` command);
   - change presentation and input options (ADR-0007 rule 2);
   - deliver keystrokes, like any input adapter.

   It never writes core state directly and never reaches a rule.

3. **Pause is a shell pause.** While the system menu is open, or the OS
   backgrounds the app, the shell stops advancing the clock. No jiffy is owed
   for the paused wall time: D-10's catch-up restarts from the resume instant.
   This is recorded as deviation D-16 in
   [`clock-and-scheduler.md` §13](../specification/clock-and-scheduler.md).
   In-play overlays — floor/pack pickers, the INCANT keyboard, `⌨`, `C U`/`C D`
   confirmation — never pause (charter, Mobile UX).

4. **Pause invariance.** Because pause only withholds jiffies, a run paused at
   any points yields the same jiffy-stamped core trace (shell `PAUSE`/`RESUME` lines excluded) as the same keystrokes
   delivered on the same jiffies without pause. Phase 8 adds this as a test.

5. **Two kinds of save, kept apart.** The menu's Save/Load use suspend
   snapshots (`DAGSNAP 1`) in shell-managed slots. Historical `ZSAVE`/`ZLOAD`
   stay commands, typed or entered through `⌨`, and keep their `DAGRAM 1`
   files. A menu load is not a game action and does not run `LOAD90`.
   This is a second use of the suspend snapshot beyond ADR-0005's "survive an
   OS kill", with two consequences the shell must handle:
   - `Game::restore_snapshot()` aborts on an unknown magic or version. The
     shell must validate a slot before restoring it and refuse a bad one.
   - The snapshot contains the in-memory cassette, so loading a slot also
     rolls back any `ZSAVE` made after it was taken. The `DAGRAM 1` files the
     desktop window already copies out are unaffected.

6. **The system menu** (decided 2026-09-27; button at lower right on touch, Esc on desktop)
   offers Resume, Save, Load, Restart, Video, Controls and Quit. Save and Load
   show five slots, each auto-named from the dungeon level and time played,
   read from the core without changing it. Saving over an occupied slot asks
   for confirmation, then saves and renames it. Restart and Quit ask for
   confirmation. A sixth, **hidden slot** (also a `DAGSNAP 1` suspend snapshot) is not listed in Save or Load. The
   shell writes it when the OS backgrounds the app and reads it at the next
   launch. The resumed game opens paused, and the hidden slot is cleared once
   play continues.

7. **Settings live in the shell** (`SDL_GetPrefPath` on desktop), never in the
   RAM image or snapshot.

8. **Parallel work is trunk-based.** There is no long-lived "pure port"
   branch and no long-lived "enhanced" branch. Isolation is in code: build
   targets, the dependency direction checked by `boundary-checker`, and the
   Original conformance suite, which `make all` runs on every PR unchanged
   (ADR-0007 rule 4). Core fixes land on `main` as small PRs with their
   reconciliation entries; feature branches merge `main` often. Rule variants
   still wait for Phase 11 policy objects.

## Consequences

- When this ADR is accepted, ADR-0007 rule 5 is narrowed: in-play pause and
  slower clocks stay enhanced-mode; shell pause does not. ADR-0007 is not
  edited while this ADR is Proposed; acceptance records the narrowing there.
- The Phase 8 prompt is rewritten around three workstreams: touch adapters,
  the shell, and render styles.
- Phase 9's backgrounding handler is a shell pause plus a snapshot to the
  hidden slot, not new machinery.
- **Trace markers** (decided 2026-09-27): for debugging, the shell writes
  `PAUSE` and `RESUME` lines. Both carry the same jiffy, the one at which
  delivery stopped. They are shell lines, not `CoreEvent`s: ADR-0004 rule 1 admits
  only events the simulation produces. That keeps the core trace diffable
  with ROM traces. The pause-invariance test compares traces with these
  lines removed.
