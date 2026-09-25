# ADR-0005 — Save/load

**Status:** Proposed. To be settled in Phase 5.

## Context

The original saves to cassette through `ZSAVE`/`ZLOAD` (`PZTAPE`, `COMMON`
`SAVE/LOAD`), writing RAM blocks. The Phase 0 report (§16) warns that a modern
save must include scheduler and RNG state, not just position. Mobile platforms
also kill backgrounded apps, which the original never faced.

## Questions Phase 5 must answer from the listing

- Exactly which RAM blocks `ZSAVE` writes, and whether queues, `SEED`, clock
  counters and `CMXLND` are among them. **Unresolved** today.
- What `ZLOAD` restores and what it re-derives (e.g. is the maze rebuilt by
  `NEWLVL` or loaded).
- What the player sees and what time elapses during a save.

## Proposed decision

1. `ZSAVE`/`ZLOAD` in Original Mode save exactly the state the original saves,
   and a loaded game continues exactly as the original would, including what it
   loses. If the original drops queue state, so does Original Mode.
2. Separately, the platform layer may take a **suspend snapshot** — the complete
   core state (everything required for bit-identical continuation) — to survive
   an OS kill. Restoring it is not a game action and is invisible to the
   simulation. It is recorded as a platform deviation, not a rule change.
3. Both formats are versioned, little-endian, and carry the core's build ID;
   an incompatible snapshot is refused rather than migrated silently.

## Consequences

- The core needs a total-state serialiser in Phase 5 anyway, for tests
  ("deterministic repeat with identical complete initial state", Phase 0 report
  §20.4).
