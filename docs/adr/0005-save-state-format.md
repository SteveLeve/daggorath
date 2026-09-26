# ADR-0005 — Save/load

**Status:** Accepted 2026-09-25.

## Resolution

Resolved 2026-09-25 from the pinned listing.

1. **What `ZSAVE` writes.** **[SRC]** `PZSAVE` only records the filename and sets `ZFLAG` to 1. `SCHED1` tests `ZFLAG` after each task is requeued and branches to `COMMON.ASM SAVE`, which writes 128-byte blocks from `DP.BEG` (`$0200`) through `MM.END`. That is the whole direct page and common RAM: the player block, lights, `FAINT`, `BAGPTR`, `FRZFLG`, `SEED`, `JIFFY`–`DAY`, the queue heads, the heart bytes, `DSPMOD`, `ZFLAG`, the keyboard buffer and its pointers, `LINBUF`, `CMXLND`, `CCBLND`, `MAZLND`, `TCBLND`, and `OCBLND`. Queues, `SEED`, the clock, and `CMXLND` are all in the image. The stack (`PDL`) and the video buffers are outside it.
2. **What `ZLOAD` restores.** **[SRC]** `LOAD` copies the image back to `$0200` and does not call `NEWLVL`, so the maze, creatures, and tasks come from the tape, not from regeneration. It then runs `LOAD90`: `IRQSYN`, `CLR ZFLAG`, `INIVU` (`HUPDAT`, then `PLOOK`), `PROMPT`, and `SCHED` from the head of `SCDQUE`.
3. **Time and display during a save.** **[SRC]** `PIATAP` disables the IRQ to the CPU, so no jiffy is counted during tape I/O and keys are not polled. `IRQSYN` then waits for the next interrupt, so the rest of the lap runs after it. The screen after either operation is the viewer.

The core applies this:

- **Historical payload** is `Game::ram_image()`: every field the core models inside `DP.BEG`–`MM.END`, including the task table, the ready and countdown lists, the keyboard buffer, the maze, and every object and creature block. Task bodies are rebuilt from their `TCBDAT` or `CMOVE-n` names, which stand in for the `P.TCRTN` pointers. The image carries a `DAGRAM 1` header because it is a text encoding of those fields, not the 6809 byte layout.
- **Suspend snapshot** is `Game::snapshot()`: the RAM image plus what lies outside it, namely the trace jiffy count, the halt state (`BRA *`), a pending `ZFLAG`, and the in-memory cassette. It is versioned `DAGSNAP 1`, and an unknown version aborts instead of migrating. Restoring it is not a game action.
- **Storage envelope** belongs to `src/platform` and is not implemented.
- **Deviation D-11** (`clock-and-scheduler.md` §13). `LOAD` searches the tape forever for a matching name. With an in-memory cassette the core reports `???` for an absent name instead.

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
