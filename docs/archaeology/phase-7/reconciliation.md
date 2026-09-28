

## Map display on the desktop (2026-09-27, refinement run 15)

Until now the window never drew the map. `project_viewer` returns no segments in mapper mode, so after a scroll `USE` the viewport was blank; the map projection was only printed by `dcli --present-map`. `rasterize_map` (`src/presentation/mapper.cpp`) now follows `MAPPER.ASM`, **[SRC]**:
- `DSP32` makes each maze cell one byte (8 pixels) by 6 scanlines, 32 × 32 cells over all 192 lines.
- A `$FF` cell is white on every line (MAPP20-22).
- With `MAPFLG` set, unowned objects on the level get `MARK4 $00/$08` and live creatures `$10/$54`.
- The player always gets `$24/$18`, and the vertical features always get `$3C/$24` (MAPP50, MAPP60).

**[INF]** the window leaves out the status line and text bands while the map is up. The map covers every scanline, and `HEARTF` and the prompt are both off in map mode (`PUSE.ASM` USC210, `HUMAN.ASM` HMAN70); whether the ROM's text routines redraw over it has not been captured. The **examine screen** is still not drawn by the window (open; `project_examine` only reaches `dcli`).

## Examine screen on the desktop (2026-09-27, refinement run 16)

The window drew nothing in examine mode, a blank viewport under the text bands. `paint_examine` (`src/presentation/examine.cpp`) now prints the `project_examine` page into the viewport. **[SRC]** `EXAMIO` runs `ZFLOP` on the alternate screen, then points `TXTEXA.TXBAS` at the current video base (`PEXAM.ASM:23-25`). `TXTEXA` is 32 × 19 characters (`COMDAT.ASM:94`), so the page covers scanlines 0–151 at 8 scanlines per row. **[INF]** the window clears only those scanlines, to 0 rather than a value derived from `VDGINV`, and leaves the status and command bands as they were; whether the ROM redraws the bands over the cleared screen has not been captured. The lit torch's whole name is printed inverse (`COM P.TXINV` before the name, `PRTOBJ` restores `VDGINV` after it). The text dump (`--present-text`, phase-6 fixtures) is unchanged. **[INF]** an inverse cell fills all 8 scanlines of the character cell.

## C-22: level-build time for CLIMB and ENDGAM (2026-09-27, refinement run 26)

**[ROM]** MAME 0.264 `coco2b`, script `docs/archaeology/phase-7/scripts/c22-climb-builds.script`,
**harness-modified**: `DOD_POKE` places the player on a `VFTTAB` feature before each `CLIMB`
(`PROW` at isr 50, 750, 1450, 2150, 2850), sets `PPOW` $7000 and clears `PDAM` so the player
survives, and writes `LEVEL` = 2 at isr 2150 so `PCLIMB`'s `ADDA LEVEL` builds level 3, which
no ladder or hole reaches. The pokes change where and which level is built, not how long a
build takes: each level's carving starts from a fixed seed.

| Build | Command line (isr) | Spin entry, `SECOND` | Command → spin | Spin draws, isr | Spin exit → first `PLAYER` |
|---|---|---|---|---|---|
| L0 → L1 | 63 | 389, 12 | 326 | 12, 2 | 22 |
| L1 → L2 | 764 | 1142, 25 | 378 | 25, 2 | 22 |
| L2 → L1 | 1463 | 1789, 36 | 326 | 36, 4 | 24 |
| L1 → L3 (poked) | 2164 | 2541, 48 | 377 | 48, 5 | 23 |
| L3 → L4 | 2871 | 3210, 59 | 339 | 59, 6 | 27 |
| `descend-early` L0 → L1 | 1775 | 2103, 41 | 328 | 41, 4 | 23 |
| `descend-early` L1 → L0 | 2280 | 2642, 50 | 362 | 50, 5 | 26 |

Level 1 is 326 in this capture and 328 in `descend-early`. The core uses 326 [ROM, ±2].
The spin costs about one interrupt per ten draws [INF from these samples].

**Applied (owner, Q6).** `CLIMB` now runs `PREPAR` and a timed `NEWLVL`
(`src/core/game.cpp` `begin_newlvl`, `build_resume`):
- At the command, `STA LEVEL`, the CCB `ZERO` and `SYSTCB`'s queue clear happen, and the
  window shows PREPARE! (`MISC.ASM` PREPAX).
- The foreground is blocked for the per-level pre-spin count (362, 326, 378, 377, 339).
- The level is then generated with the `SECOND` current at that point.
- The system TCBs are queued, then the creatures are born and queued.
- A second block covers the spin and the tail (26, 23, 22, 23, 27 per level, with
  `INIVU`'s `SYNC` as its last jiffy). `HMAN70` then prompts.

`ENDGAM`'s `NEWLVL 3` uses the same timing. The screen keeps the old level's polarity until
the build's `NLVL50`.

**Check against `descend-early`:** the core now builds level 1 at `SECOND` 41 (spin at jiffy
2102 against isr 2103) with the ROM's creatures at the ROM's cells. The return to level 0
builds at `SECOND` 50 and matches too. `CMXLND` evolves byte for byte as in the capture, with
`CREGEN` at jiffies 2130 and 2674 against isr 2133 and 2673.

An intermediate version queued `SYSTCB`'s tasks at the command. `CREGEN` then ran before the
births and added a creature. The capture caught this: `SYSTCB`'s TCBs wait in `Q.SCD` until
`PLAYER` returns from the whole build.

**Baselines:** no committed trace contains a `CLIMB`. The object and progression tests now
wait for the build and check 326 and 83 + 377 from the listing and the capture. The
disabled playthrough will move (Phase 5b).
