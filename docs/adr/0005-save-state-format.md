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

Three distinct things, kept separate:

1. **Historical payload.** The bytes `ZSAVE` writes and `ZLOAD` reads. Their
   layout and semantics are source-derived in Phase 5 and are not modified: no
   header, version field or reordering inside them. A loaded game continues
   exactly as the original would, including what it loses. If the original drops
   queue state, so does Original Mode.
2. **Storage envelope (optional).** A modern wrapper the platform layer puts
   around a historical payload to store it as a file (schema version, checksum,
   which slot). It is outside the payload, invisible to the simulation, and
   changes no game-visible save behaviour.
3. **Suspend snapshot.** The complete core state (everything required for
   bit-identical continuation), taken by the platform to survive an OS kill.
   Restoring it is not a game action. It is its own versioned format, recorded
   as a platform deviation, not a rule change.

The envelope and the snapshot carry a **schema version**; an incompatible one
is refused rather than migrated silently. Byte order, and whether a build
identifier is also recorded, are decided in Phase 5 after the historical
payload layout is known; no requirement is placed on them here.

## Consequences

- The core needs a total-state serialiser in Phase 5 anyway, for tests
  ("deterministic repeat with identical complete initial state", Phase 0 report
  §20.4).
