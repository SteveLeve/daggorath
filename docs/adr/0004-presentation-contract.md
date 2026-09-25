# ADR-0004 — Presentation contract

**Status:** Proposed. Contract drafted in Phase 6a (any time after Phase 2),
settled in Phase 6.

## Context

The charter's conceptual API has `game.renderState()`. The Phase 0 report (§14)
notes that `TURN` and `MOVE` animations and `SYNC` waits change game timing,
that sound generation runs synchronously in the foreground, and that
`MAPPER`/`EXAMIN` are distinct display modes. Today the only output is the
`dcli` trace. Phases 2–5 will produce creature sounds, hit sounds, messages and
mode changes; if each invents a trace line, Phase 6 must retrofit them.

## Proposed decision

1. The core exposes two outputs, both pure data with no platform types:
   - `RenderState` — a snapshot sufficient to draw the current frame: display
     mode, player pose, visible cells and what they hold, light level, status
     line fields, text region contents, heart indicator state.
   - an ordered stream of `PresentationEvent`s stamped with the jiffy and the
     scheduler position that emitted them: sound cues (type, source volume),
     text output, animation start (kind, and blocking duration where known),
     mode change.
2. Blocking is a **core** concern: an event that blocks the foreground in the
   original does so in the core, for the number of jiffies the spec gives. Where
   the spec says unresolved (D-4), the duration is 0 and the event carries
   `duration_known = false`.
3. The renderer and audio are functions of `RenderState` and events; they never
   read core internals and never feed anything back except input.
4. The `dcli` trace prints presentation events in a stable text form, so ROM
   traces and core traces stay diffable.

## Consequences

- Phases 3–5 emit events through the contract, not ad hoc trace lines, once 6a
  lands. Until then they emit trace lines named so they map one-to-one.
- The same `RenderState` drives the golden-image tests of Phase 7.
