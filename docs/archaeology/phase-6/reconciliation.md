# Phase 6 reconciliation

`dcli --present` prints a logical draw list. `SNOISE` does not consume `SEED`.

The draw list uses the `NORSCL` bytes on a facing ray. `VARC`, `VERT`, `VOBJ`, `D3`, `D4`, and `VCTLST` are not decoded yet. Sound-loop durations inside `SOUNDS.ASM` remain unresolved (D-4). Screenshot and audio captures for fixed states are not in the capture backlog as completed rows.

## Mapper, examine and text

Screen regions are **[SRC]** `COMDAT.ASM` `STSVDB`/`PRIVDB`/`VCNTRX`/`VCNTRY`: centroid (128, 76), viewer through scanline 152, status through 160, command through 192. The Phase 0 report's line numbers match those constants.

`MAPPER.ASM` is a 32×32 occupancy dump (solid `$FF` versus empty) plus optional object/creature marks, not the door-colour legend in its comment. `EXAMIN` and `STATUX` are character-cell projections. Fixtures live in [`fixtures/text/`](fixtures/text/) with their own `MANIFEST.json`, produced by `tools/extract_text.py`.
