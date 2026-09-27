

## Map display on the desktop (2026-09-27, refinement run 15)

Until now the window never drew the map. `project_viewer` returns no segments in mapper mode, so after a scroll `USE` the viewport was blank; the map projection was only printed by `dcli --present-map`. `rasterize_map` (`src/presentation/mapper.cpp`) now follows `MAPPER.ASM`, **[SRC]**:
- `DSP32` makes each maze cell one byte (8 pixels) by 6 scanlines, 32 × 32 cells over all 192 lines.
- A `$FF` cell is white on every line (MAPP20-22).
- With `MAPFLG` set, unowned objects on the level get `MARK4 $00/$08` and live creatures `$10/$54`.
- The player always gets `$24/$18`, and the vertical features always get `$3C/$24` (MAPP50, MAPP60).

**[INF]** the window leaves out the status line and text bands while the map is up. The map covers every scanline, and `HEARTF` and the prompt are both off in map mode (`PUSE.ASM` USC210, `HUMAN.ASM` HMAN70); whether the ROM's text routines redraw over it has not been captured. The **examine screen** is still not drawn by the window (open; `project_examine` only reaches `dcli`).
