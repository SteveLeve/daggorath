# Phase 6 reconciliation

`dcli --present` prints a logical `VIEWER` draw list. `SNOISE` does not consume `SEED`.

Vector lists, `NORSCL`/`HLFSCL`/`BAKSCL`, `FLATAB`, `FWDOBJ`, `FWDCRE`, and `BITMSK` are extracted by `tools/extract_vectors.py` into `docs/archaeology/phase-6/fixtures` and consumed through generated `vector_tables.hpp`. `tools/viewer_ref.py` is a second transliteration of `VIEWER`/`VCTLST`; `tests/conformance/viewer_regressions.cpp` matches its draw-list files for the start cell of every level in darkness, regular light 7, and magic light 13.

Sound-loop durations inside `SOUNDS.ASM` remain unresolved (D-4b). Screenshot and audio captures for fixed states are not in the capture backlog as completed rows. Half-step `HLFSCL`/`BAKSCL` frames are extracted and not drawn.

## Phase 6a — CoreEvent stream (2026-09-25)

Default `dcli` traces (`t1`–`t5`, `idle-10min`, `fight-to-kill`, `fight-to-death`) are **not** regenerated. The `SOUND` and `SYNC` lines stay in their previous form so Phase 0b/2/3 identity checks keep working. The stable CoreEvent print is `dcli --events`.

No default-trace line is renamed in this slice. Heartbeat toggles, `Block` events, `MODE` stores, and detailed sound payloads live only on the CoreEvent stream.

`fixtures/sounds.json` is a hashed fixture (cue table and generator immediates). `make fixtures` extracts it after the Phase 0b set.
