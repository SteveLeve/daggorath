# Roadmap — phases after Phase 1

Written 2026-09-25, while Phase 1 was software-complete on
`cursor/phase-1-level-population`. This is a plan, not a specification: nothing
here is a behavioural claim. Where it names a listing routine as the subject of a
phase, the routine names come from the Phase 0 report §9–16 and the Phase 0b and
Phase 1 reconciliations; what each routine *does* is for the phase to establish
and label.

The charter (`docs/project-instructions.md`) wins over this file. The implementation
order in charter §6 is kept; this file splits step 2 ("headless simulation core")
into gameplay phases small enough for one agent session each, and adds a
cross-cutting evidence track.

Decisions that shape more than one phase are recorded as ADRs in
[`../adr/`](../adr/README.md).

## 1. Where Phase 1 leaves the project

| Area | State at end of Phase 1 |
|---|---|
| Cartridge bytes | Listing at `a94326f` assembles byte-identical to catalog 26-3093 |
| Runtime evidence | MAME 0.264 `coco2b` captures in [`../archaeology/phase-1/reconciliation.md`](../archaeology/phase-1/reconciliation.md) §1. A claim is ROM-observed only where that section says so |
| Core | RNG, all five mazes, clock/scheduler, keyboard buffer, line editor, parser, `MOVE`/`TURN`/`LOOK`, heart rate, `NEWLVL` population, `CREGEN` |
| Deviations | D-1…D-5 open; D-6 (`CMOVE` not queued) waits for creature movement |
| Everything else | Reports `UNIMPLEMENTED` |

## 2. Phase sequence

| Phase | Name | Charter step | Depends on | Prompt |
|---|---|---|---|---|
| 1 | ROM conformance harness, level population, regeneration | 1–3 | 0b | [`phase-1-…`](../prompts/phase-1-conformance-and-creatures.md) — complete |
| 2 | Creature movement (`CMOVE` without attack), scheduler lap model | 2 | 1 | [`phase-2-creature-movement.md`](../prompts/phase-2-creature-movement.md) |
| 6a | Core event contract (ADR-0004 core half) | 4 | 2 | [`phase-6a-core-events.md`](../prompts/phase-6a-core-events.md) |
| 3 | Combat, physiology to death | 2 | 2, 6a | [`phase-3-combat.md`](../prompts/phase-3-combat.md) |
| 4 | Objects, inventory, torches, magic, vertical travel | 2 | 3 | [`phase-4-objects-and-magic.md`](../prompts/phase-4-objects-and-magic.md) |
| 5 | Progression, endings, save/load — **headless game complete** | 2–3 | 4 | [`phase-5-progression-and-save.md`](../prompts/phase-5-progression-and-save.md) |
| 6 | Presentation state: viewer, mapper, text, sound events | 4 | 5 (contract may start after 2) | [`phase-6-presentation-state.md`](../prompts/phase-6-presentation-state.md) |
| 7 | Desktop SDL3 application: vector renderer and audio | 4–5 | 6 | [`phase-7-desktop-sdl.md`](../prompts/phase-7-desktop-sdl.md) |
| 8 | Touch input layer (on desktop first) | 6 | 7 | [`phase-8-touch-input.md`](../prompts/phase-8-touch-input.md) |
| 9 | Android and iOS packaging | 7–8 | 8, licensing review | [`phase-9-mobile-packaging.md`](../prompts/phase-9-mobile-packaging.md) |
| 10 | Mobile UX refinement | 9 | 9 | outline only (§4) |
| 11 | Optional enhanced modes | 10 | 10, ADR-0007 | outline only (§4) |
| R | ROM observation track (cross-cutting) | 1 | 1 | [`track-r-rom-observation.md`](../prompts/track-r-rom-observation.md) |

Why this order (ADR-0001): each gameplay phase adds one family of foreground or
creature tasks to the scheduler, so a divergence can be attributed to one
subsystem. Movement precedes combat because `CMOVE` is the task that *calls* the
creature attack; combat precedes objects because `PATTK` is where the empty-hand,
ring and torch gates live and because kill drops loot; progression comes last
because both endings are reached through combat and `INCANT`.

```text
1 ─► 2 ─► 6a ─► 3 ─► 4 ─► 5 ─► 6 ─► 7 ─► 8 ─► 9 ─► 10 ─► 11
                                    R runs alongside 2–7 and back-fills labels
```

## 3. Phase scope summaries

Each phase's full prompt is under `docs/prompts/`. Every prompt follows the
conventions in `docs/prompts/README.md`: preservation requirement stated, working
artifacts not plans, a named completion gate with run output, an explicit
"do not build" list, and unverified claims left unresolved.

### Phase 2 — Creature movement

- **In:** `CMOVE` priorities except the attack branch (frozen skip, dead removal,
  object pickup, line-of-sight approach, random preference turns, back-off),
  requeue at movement/attack delays, queuing at `CBIRTH` (retires D-6), the
  scheduler lap decision for D-1/D-2 (ADR-0002), creature sound *events* as
  data only.
- **Out:** damage of any kind, the attack branch's effect (a creature that would
  attack emits an `ATTACK-PENDING` trace event and does nothing — recorded as a
  deviation), rendering.
- **Gate evidence:** multi-minute level-0/level-4 traces with creature positions
  per jiffy; tie-order fixtures for simultaneous creature expiries.

### Phase 3 — Combat

- **In:** `PATTK` (player attack, exertion, sound event, torch/darkness gate,
  ring bypass), `ATTACK`, `DAMAGE`, `SCAL16` with exact byte widths, creature
  attack branch of `CMOVE`, kill (loot drop, `CMXLND` decrement in `PATT40`),
  player damage, faint, recovery, and the simulation transition to dead
  (`DEATH`): Phase 3 owns it completely.
- **Out:** wizard kill endings (Phase 5: the kill is detected and traced, the
  `ENDGAM` body is `UNIMPLEMENTED`), item use, rendering.
- **Gate evidence:** exhaustive/property fixtures for `SCAL16` and `DAMAGE` over
  every creature × weapon pair; `ATTACK` outcome tables per RNG byte.

### Phase 4 — Objects and magic

- **In:** `GET`, `DROP`, `STOW`, `PULL`, `EXAMINE` (as state, not display),
  `REVEAL`, `USE` (flasks, scrolls), `INCANT` ring transforms, `BURNER` torch
  lifecycle and light levels, `CLIMB` and vertical features (player level
  change through `NEWLVL`), burden.
- **Out:** `ENDGAM`/`WINNER` effects, map display, `ZSAVE`/`ZLOAD` (`PZTAPE`).
- **Gate evidence:** torch minute-boundary fixture; every token in `tokens.json`
  now reaches a handler or a recorded `UNIMPLEMENTED` owner phase.

### Phase 5 — Progression, endings, save/load

- **In:** `ENDGAM` (wizard image kill on level 2 → level 3 regeneration and
  relocation), true-wizard kill freeze, final ring and `WINNER`, any
  death-related progression or dialogue state Phase 3 deferred (not the
  transition itself; drawing it is Phase 6), the save/load decision (ADR-0005) and its implementation.
- **Exit criterion for the headless game:** a scripted complete playthrough
  from power-on to `WINNER` runs in `dcli` with a committed trace, and no
  command reports `UNIMPLEMENTED`.

### Phase 6 — Presentation state

- **In:** the presentation-owned `RenderState` projection over core state and
  Phase 6a's `CoreEvent`s (ADR-0004),
  `VIEWER` visibility and draw list as data, `MAPPER` modes, status and text
  regions, extraction of vector geometry (`VARC`, `VERT`, `VOBJ`, `D3`, `D4`,
  `VCTLST`) and scale tables as fixtures (ADR-0006), sound events with source
  volume, `SNOISE`'s effect on `SEED`, and a resolution of D-4 (blocking
  durations) as far as the listing allows.
- **Out:** pixels, SDL, audio output.
- **Phase 6a (required, small):** the core-owned `CoreEvent` types from
  ADR-0004 land after Phase 2 and before Phase 3, so Phases 3–5 emit events into
  them instead of ad hoc trace lines. Prompt:
  [`phase-6a-core-events.md`](../prompts/phase-6a-core-events.md). Phase 6
  itself builds only the presentation-owned projection.

### Phase 7 — Desktop SDL3 application

- **In:** `src/platform` SDL3 window, 256×192 logical surface, vector
  rasteriser faithful to `VECTOR`'s DDA, DAC waveform synthesis from `SOUNDS`,
  keyboard adapter emitting timestamped keystrokes, 60 Hz pacing that never
  skips jiffies (spec §12 obligation 1).
- **Gate evidence:** golden-image tests at fixed states rendered headlessly
  (offscreen), audio sample hashes per sound event.

### Phase 8 — Touch input

- **In:** `src/input` adapters that emit keystroke streams or parsed commands
  (never a side door), prototyped in the desktop app with mouse-as-touch,
  portrait and landscape layouts evaluated, visible command trace option.
- **Out:** any rule change. Pausing only where the original pauses.

### Phase 9 — Mobile packaging

- **Precondition:** licensing questions 1, 4, 5 in `docs/licensing/README.md`
  have a recorded answer or a recorded decision to build privately only.
- **In:** Android (Gradle + CMake) and iOS (Xcode via CMake) shells around the
  Phase 7 app; lifecycle (background = what? — decide and record as a
  deviation), storage path for saves, CI builds.

## 4. Outline-only phases

**Phase 10 — Mobile UX refinement.** Controller and hardware-keyboard support,
accessibility that does not alter Original Mode timing (e.g. scaling, contrast
of the vector lines), onboarding using the original manual's structure only if
its rights are settled. Write its prompt after Phase 9's gate.

**Phase 11 — Enhanced modes.** Seeded random maps, rule variants, modern audio.
Only through the extension points in ADR-0007; Original Mode's conformance suite
must pass unchanged with every enhanced mode compiled in.

## 5. Track R — ROM observation

Phase 1 proved the bytes and recorded the captures in
[`../archaeology/phase-1/reconciliation.md`](../archaeology/phase-1/reconciliation.md) §1.
A rule is ROM-observed only where that section says so. Track R continues for
the rows still open in [`capture-backlog.md`](capture-backlog.md), without
blocking gameplay phases (ADR-0003). Further firmware paths (a dump from the
owner's hardware, a licensed distribution, or a clean-room replacement BASIC)
are recorded in the ledger before use.

Each gameplay phase adds its capture requests to the track backlog
(`docs/planning/capture-backlog.md`) instead of blocking on them.

## 6. Cross-phase rules for agents

1. Open exactly one phase at a time; update `README.md` "State" and
   `docs/prompts/README.md` when opening and closing it.
2. A phase closes with `docs/archaeology/phase-N/reconciliation.md` and a
   forward pointer appended to the previous phase's reconciliation.
3. A command not yet in scope keeps reporting `UNIMPLEMENTED`. Never
   approximate it.
4. A new deviation gets a D-number in `clock-and-scheduler.md` §13 with the
   phase that must retire it.
5. The review agent for every phase uses
   [`review-checklist.md`](review-checklist.md).
