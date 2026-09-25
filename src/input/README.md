# input

Adapters that turn a physical input into the original command language. Nothing
here bypasses the command model.

Rules for this module:

- Every adapter produces either a timestamped keystroke or a parsed command;
  there is no third path into the simulation.
- Keystrokes are timestamped individually, not per completed command. The
  original's 32-byte keyboard buffer has no overflow check, so bursts and
  saturation are observable behaviour that per-command timestamps would erase.
- Touch and controller mappings are presentation-layer conveniences that resolve
  to the same commands: `Forward -> MOVE`, `Turn Left -> TURN LEFT`,
  `tap weapon -> ATTACK LEFT|RIGHT`.

First work here: a keyboard adapter shared by the desktop app and `dcli`, then a
replay adapter that reads the trace script format in
`docs/archaeology/phase-0b/traces/`.
