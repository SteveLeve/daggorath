# ADR-0008 — Stubbing behaviour owned by a later phase

**Status:** Accepted, 2026-09-25.

## Context

Phases split the game along task families, but the original's routines do not
respect that split: `CMOVE` can attack, `PATTK` can end the game, a kill drops
objects. Phase 1 already set the pattern with D-6 (not queuing `CMOVE`) and
`UNIMPLEMENTED` for commands.

## Decision

When a routine in scope reaches a branch owned by a later phase:

1. **Commands** still report `UNIMPLEMENTED` and consume exactly what the
   parser consumes today; they do not approximate.
2. **In-scope routines** execute every side effect up to the branch point that
   the source performs before it (RNG draws, exertion, requeue), then emit a
   trace event naming the deferred branch (e.g. `DEFER creature-attack`), and
   take no further effect.
3. Each stub is a numbered deviation in `clock-and-scheduler.md` §13 with the
   phase that retires it, and a test that asserts the stub fires, so retiring it
   is a visible diff.
4. A stub must not be placed where it would change RNG draw count or task order
   relative to the source without saying so in its deviation row.

## Consequences

- Traces from a phase stay diffable against later phases: the stub line is
  replaced, not moved.
