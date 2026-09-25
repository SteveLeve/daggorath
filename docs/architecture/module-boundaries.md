# Module boundaries

Status: the core is real; `presentation`, `input` and `platform` are placeholders
with interface targets so dependents can be wired before the implementations land.

```text
        platform (SDL3, storage, packaging)
             |            ^
             v            |
  input adapters      presentation
        \                 ^
         \                |
          ->  command interface
                    |
                    v
             daggorath::core
          simulation state + clock
```

Dependencies point inward. `core` is the innermost layer and links nothing.

## `src/core` — the simulation

Owns: the 60 Hz clock and counters, countdown queues and the ready queue, the
24-bit RNG, maze generation and the world, entities, the command language and
parser, player physiology, and the presentation *state* the original couples to
timing (`UPDATE`, display mode, pending redraw, heartbeat flags).

Constraints:

- Links nothing. No SDL, no platform headers, no I/O beyond the standard library.
- Runs headless and deterministically: given initial state, the level seed, a
  timestamped keystroke sequence and an elapsed jiffy count, it reproduces the
  same run.
- `advanceJiffies(n)` executes **n discrete boundaries**. A large delta never
  skips intervening interrupt work.
- Pointer identity in the original becomes stable entity IDs with ordered lists.
  Order is behaviour, so lists, never sets.

Why the coupled flags live here: the original's `HEARTF` gates the heart glyph
*and* marks map mode, which changes how input is handled; `UPDATE` is consumed by
the interrupt; `SYNC` inside a command spends a jiffy. Treating these as "only
graphics" and moving them outward would change timing.

## `src/presentation` — derived, observable consequences

Visibility, display mode, ordered screen and audio events, timed transitional
frames. Reads core state, never mutates it. Owns no rendering API. See
[`../../src/presentation/README.md`](../../src/presentation/README.md).

## `src/input` — adapters to the command language

Touch, keyboard and controller all resolve to the same commands. Keystrokes are
timestamped individually, because the original's unchecked 32-byte keyboard buffer
makes bursts and saturation observable. See
[`../../src/input/README.md`](../../src/input/README.md).

## `src/platform` — everything the simulation must not know

SDL3, rendering and audio backends, filesystem and save storage, Android and iOS
packaging. Save state must include scheduler and RNG state, not just position. See
[`../../src/platform/README.md`](../../src/platform/README.md).

## `src/app` — entry points

`dcli` today: runs timestamped input scripts and emits comparable traces. The
desktop SDL application belongs here later as a separate target.

## `tests` — conformance

Cross-checks against the extracted fixtures rather than against the
implementation's own output. The maze test reads fixture bytes off disk, so it
compares two independent transliterations of the same assembly routine.

## Rules that keep the boundary honest

1. A new dependency on `core` from an outer module is normal; the reverse is a
   defect.
2. Anything that changes *when* something happens belongs in `core`, whatever it
   looks like.
3. Original Mode is a ruleset, not a build flag scattered through the code. Future
   modes attach at extension points; they never edit Original Mode in place.
4. Deviations from the original are recorded in
   [`../specification/clock-and-scheduler.md`](../specification/clock-and-scheduler.md)
   §13, not smoothed over in code.
