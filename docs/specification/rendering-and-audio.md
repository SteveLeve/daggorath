# Rendering and audio

## Surface

**[SRC]** Logical coordinates are 256 dots wide. `COMDAT.ASM` RAMDAT stores `VCNTRX` = 128 and `VCNTRY` = 76. `CD.ASM` `D0.LEN` is `32*19*8` scan lines, so the graphics viewport ends at line 152. `COMDAT.ASM` `STSVDB` names the 152nd and 160th scan lines; `PRIVDB` names the 160th and 192nd. Status occupies 152–160; the command region occupies 160–192.

## Viewer

**[SRC]** `VIEWER.ASM` walks `RANGE` from 0 through 9. At each cell it unpacks the four two-bit edge fields into `NEIBOR`, indexes them by `PDIR`, and draws `SWCHAR.ASM` `FLATAB` in left, forward, right order. Feature codes are `HF.PAS` 0, `HF.DOR` 1, `HF.SDR` 2, `HF.WAL` 3 (`CD.ASM`). A secret door is drawn with magic light, then replaced by a wall drawn with regular light.

**[SRC]** Draw order in one cell is architecture, then the creature `CFIND` returns, then peek-a-boo (`PDRAW`: a passage on that side and a creature in the stepped-to cell, using `LPEEK`/`RPEEK` from `VARC.ASM`), then `VFIND` (or `CELINE` when none), then unowned objects from `OFIND`. Objects are submitted to `DRAWIT` twice: magic light, then regular light.

**[SRC]** Line of sight stops when the forward edge is not a passage. Doors and secret doors block the walk. After a step, `RANGE` increments; `CMPA #9 / LBLE VIEW10` draws at most ranges 0–9.

**[SRC]** `SETSCL` loads `NORSCL` for a standing look: 200, 128, 80, 50, 31, 20, 12, 8, 4, 2 in 128ths (`VIEWER.ASM`). `HLFSCL` and `BAKSCL` are extracted and unused until half-step turn/move animation.

**[SRC]** `VCTLST.ASM` `SETFAX`: fade 0 when `light - 7 - RANGE >= 0`, fade `$FF` (draw nothing) when that difference is `<= -7`, otherwise `BITMSK+8` indexed by the negative remainder (`VECTOR.ASM` `BITMSK` is `BIT7`…`BIT0`). Regular architecture uses `RLIGHT`. `MAGFLG` selects `MLIGHT` for secret-door marks, magical creatures (`P.CCMGO` nonzero), peeks of those creatures, and the first of the two object draws.

**[SRC]** Vector lists live in `VARC.ASM`, `VERT.ASM`, `VOBJ.ASM`, `D3.ASM`, `D4.ASM`. `V$FF` starts a new pen, `$FE` ends, `$FC` relative nybbles, `$FB`/`$FA` subroutine, `$FD` jump. This projection records logical endpoints, not rasterised pixels.

**[INF]** Map and examine modes emit `MAP features` / `MAP plain` and `EXAMINE` with an empty draw list. Those screens are `MAPPER.ASM` / `PEXAM.ASM`, not `VIEWER`.

**[OPEN]** `HLFSTP`/`BAKSTP` half-step frames, `VECTOR` DDA and fade-to-pixels, `MAPPER`/`EXAMIN` geometry, font blit, and `SOUNDS` loop counts (D-4) are not in this draw list.

**[SRC]** `SOUNDS.ASM SNOISE` updates `SNDRND` only. It does not read or write `SEED`.

D-4 durations: the `BURNER` and `HSLOW` countdowns are source-derived (one minute, and the heart-rate delay). Sound loop counts inside `SOUNDS.ASM` are not yet transcribed, so those durations stay unresolved.
