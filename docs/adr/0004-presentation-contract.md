# ADR-0004 — Presentation contract

**Status:** Proposed. Ownership rules below are fixed now; the core event
types are settled in Phase 6a (after Phase 2, before Phase 3); the derived
projection is settled in Phase 6.

## Context

The charter's conceptual API has `game.renderState()`. The Phase 0 report (§14)
notes that `TURN` and `MOVE` animations and `SYNC` waits change game timing,
that sound generation runs synchronously in the foreground, and that
`MAPPER`/`EXAMIN` are distinct display modes. Today the only output is the
`dcli` trace. Phases 2–5 will produce creature sounds, hit sounds, messages and
mode changes; if each invents a trace line, Phase 6 must retrofit them.

## Proposed decision

Ownership follows `docs/architecture/module-boundaries.md`, which this ADR does
not change: `src/core` owns state the original couples to timing;
`src/presentation` derives everything else and never mutates core.

1. **Core-owned, neutral types** (header under `src/core/include/daggorath/`,
   no presentation includes): an ordered stream of `CoreEvent`s stamped with
   the jiffy and the scheduler position that emitted them. Only events the
   simulation itself produces or that affect its timing belong here: sound cue
   requested (cue id, the source's volume inputs), text emitted, display-mode
   change, animation or sound that blocks the foreground (kind, blocking jiffies
   where known), heartbeat toggle. Plus the timing-coupled flags the boundary
   document already assigns to core (`UPDATE`, display mode, pending redraw,
   heartbeat flags).
2. **Blocking is a core concern.** An event that blocks the foreground in the
   original does so in the core, for the jiffies the spec gives. Where the spec
   says unresolved (D-4), the duration is 0 and the event carries
   `duration_known = false`.
3. **Presentation-owned, derived state:** `RenderState` (visible cells and
   their contents, light, draw list, status and text regions) is a **pure
   projection** computed in `src/presentation` from a read-only core snapshot
   plus the `CoreEvent` stream. Core never computes it and never includes a
   presentation header. If the projection needs a core value that is not
   exposed, core exposes the value, not the projection.
4. The renderer and audio are functions of `RenderState` and `CoreEvent`s;
   nothing flows back except input.
5. The `dcli` trace prints `CoreEvent`s in a stable text form so ROM and core
   traces stay diffable. Presentation projections get their own fixtures in
   Phase 6.

**Resolution check (to do in 6a):** confirm from the listing that each event
kind in rule 1 is emitted inside simulation code (`SOUNDS` callers, `PUPDAT`,
`SYNC` waits, `CLOCK` heartbeat) and not only in display code; move any that
are display-only to presentation.

## Consequences

- Phase 6a lands before Phase 3, so Phases 3–5 emit `CoreEvent`s instead of
  ad hoc trace lines. Its prompt is `docs/prompts/phase-6a-core-events.md`.
- The same `RenderState` projection drives the golden-image tests of Phase 7.
- `boundary-checker` enforces rule 3 mechanically: no `src/presentation`
  include from `src/core`.
