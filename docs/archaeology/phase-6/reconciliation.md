# Phase 6 reconciliation

`dcli --present` prints a logical draw list. `SNOISE` does not consume `SEED`.

The draw list uses the `NORSCL` bytes on a facing ray. `VARC`, `VERT`, `VOBJ`, `D3`, `D4`, and `VCTLST` are not decoded yet. Sound-loop durations inside `SOUNDS.ASM` remain unresolved (D-4b). Screenshot and audio captures for fixed states are not in the capture backlog as completed rows.

## Phase 6a — CoreEvent stream (2026-09-25)

Default `dcli` traces (`t1`–`t5`, `idle-10min`, `fight-to-kill`, `fight-to-death`) are **not** regenerated. The `SOUND` and `SYNC` lines stay in their previous form so Phase 0b/2/3 identity checks keep working. The stable CoreEvent print is `dcli --events`.

No default-trace line is renamed in this slice. Heartbeat toggles, `Block` events, `MODE` stores, and detailed sound payloads live only on the CoreEvent stream. Adding `HEART` lines to the default traces would not be a rename.

`fixtures/sounds.json` is a new hashed fixture (cue table and generator immediates). `make fixtures` extracts it after the Phase 0b set. Manifest growth is this file only.
