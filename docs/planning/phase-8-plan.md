# Phase 8 plan — touch input, the shell, render styles

Written 2026-09-27. Planning note, not evidence. Prompt:
[`../prompts/phase-8-touch-input.md`](../prompts/phase-8-touch-input.md).
Architecture: [ADR-0009](../adr/0009-layering-and-shell.md) (layering, shell
pause, parallel work), [ADR-0010](../adr/0010-render-styles.md) (crisp vectors).
Design direction: [`../design/touch-controls/`](../design/touch-controls/README.md).

## Goal

A desktop build that plays like the phone will: touch overlay driven by mouse,
a system menu that pauses, crisp vectors by default — with the Original
conformance suite unchanged. Android/iOS packaging stays Phase 9.

## What stays fixed

- `src/core` gains nothing for this phase unless a missing read-only accessor is
  found; that would be its own PR with a boundary check.
- Touch produces keystrokes. The typed command line stays authoritative.
- In-play overlays cost game time. Only the shell menu and backgrounding pause.

## Workstreams

Each is one or more small PRs to `main`, in this order; 8.2 and 8.3 are
independent of 8.1 and can run in parallel with it.

| # | Work | Output | Gate |
|---|---|---|---|
| 8.0 | Interaction inventory from `commands-and-parser.md`, item and combat specs | Coverage table: every command form → keystrokes → control | Reviewed table in `docs/architecture/touch-input.md` |
| 8.1 | `src/input` gesture → keystroke adapters; line-at-once vs paced keystrokes (32-byte buffer quirk) | Adapter library, no SDL; deviation entry if not typist-paced | Per-gesture keystroke fixtures |
| 8.2 | Shell: pause/resume jiffy delivery; menu Resume · Save slot · Load slot · Restart · Video · Controls · Quit; Esc on desktop | Shell code in `src/platform` (or `src/shell`) | Pause-invariance test (ADR-0009 §4); snapshot slot round-trip |
| 8.3 | `crisp` renderer and style toggle; inventory of every drawn frame kind | Segment list from presentation, SDL3 line drawing | Segment fixtures per golden state; goldens unchanged |
| 8.4 | Touch overlay prototype, mouse-as-touch, per the mockups; phone landscape and tablet 4:3; optional command trace | Overlay in the desktop app | Manual evaluation notes; screenshots out of tree |
| 8.5 | Replay equivalence and write-up | `docs/architecture/touch-input.md` with layouts evaluated | Touch session ≡ keystroke transcript trace; `make all` |

## Decisions already taken (2026-09-27)

- Pause is a shell pause (ADR-0009, D-16); overlays never pause.
- `crisp` is the default render style; `pixel` is the conformance reference.
- Menu Save/Load use suspend snapshots in slots; `ZSAVE`/`ZLOAD` stay commands.
- Parallel port fixes land on `main`; no long-lived split branches.

## Open questions (resolve before or inside the named workstream)

1. **8.1** Line-at-once or paced keystrokes. Paced matches a typist and keeps
   the buffer quirk observable; line-at-once is what the web pad does.
2. **8.4** Tablet 4:3 layout: no side margins at full height.
3. **8.4** Floor/pack pickers must not show what the original would not.
4. **8.3** Text in `crisp`: scaled glyph bitmap or stroked glyphs; map style.
5. **8.2** Where the system-menu button sits on touch; snapshot slot count,
   naming, storage path; whether a shell pause leaves a trace line.
6. **8.2** Does Restart ask for confirmation; does Quit auto-snapshot.
7. **Phase 9 hook** Backgrounding = shell pause + automatic snapshot.
