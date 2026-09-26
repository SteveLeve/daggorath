# Phase 6 reconciliation

`dcli --present` prints a logical `VIEWER` draw list. `SNOISE` does not consume `SEED`.

Vector lists, `NORSCL`/`HLFSCL`/`BAKSCL`, `FLATAB`, `FWDOBJ`, `FWDCRE`, and `BITMSK` are extracted by `tools/extract_vectors.py` into `docs/archaeology/phase-6/fixtures` and consumed through generated `vector_tables.hpp`. `tools/viewer_ref.py` is a second transliteration of `VIEWER`/`VCTLST`; `tests/conformance/viewer_regressions.cpp` matches its draw-list files for the start cell of every level in darkness, regular light 7, and magic light 13.

Sound-loop durations inside `SOUNDS.ASM` remain unresolved (D-4). Screenshot and audio captures for fixed states are not in the capture backlog as completed rows. Half-step `HLFSCL`/`BAKSCL` frames are extracted and not drawn.
