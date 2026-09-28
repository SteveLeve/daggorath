# input

Adapters that turn a physical input into the original command language. Nothing
here bypasses the command model.

Rules for this module:

- Every adapter produces either a timestamped keystroke or a parsed command;
  there is no third path into the simulation.
- Keystrokes are timestamped individually, not per completed command. The
  original's 32-byte keyboard buffer has no overflow check, so bursts and
  saturation are observable behaviour that per-command timestamps would erase.
  Touch gestures are the one deliberate exception: see "Gesture adapters"
  below (D-17).
- Touch and controller mappings are presentation-layer conveniences that resolve
  to the same commands: `Forward -> MOVE`, `Turn Left -> TURN LEFT`,
  `tap weapon -> ATTACK LEFT|RIGHT`.

First work here: a keyboard adapter shared by the desktop app and `dcli`, then a
replay adapter that reads the trace script format in
`docs/archaeology/phase-0b/traces/`.

## Gesture adapters (Phase 8.1)

`gesture.hpp`/`gesture.cpp` turn a touch control (per
`docs/design/touch-controls/README.md` and the coverage table in
`docs/architecture/touch-input.md`) into a `GestureLine`: the command text a
typist would enter, delivered as `dag::KeyEvent`s all stamped on the same
jiffy (deviation D-17, `docs/specification/clock-and-scheduler.md` §13). This
differs from typed input, which stays timestamped one character per jiffy.
Every gesture line is capped at `kMaxGestureLine` (31 characters including the
CR) so a touch gesture can never itself produce the 32-byte keyboard-buffer
overrun; that stays reachable only by typing.
