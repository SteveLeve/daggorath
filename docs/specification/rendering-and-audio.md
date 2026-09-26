# Rendering and audio

## Surface

**[SRC]** `COMDAT.ASM` `VCNTRX`/`VCNTRY` store the centroid at (128, 76). `STSVDB` runs from `D0$BAS+152*32` to `D0$BAS+160*32`, and `PRIVDB` from `D0$BAS+160*32` to `D0$BAS+192*32`. The viewer occupies scanlines 0–151, status 152–159, and the command band 160–191. `CD.ASM` `D0.LEN` is `32*19*8`, the same 152-line viewer.

**[SRC]** Logical coordinates are 256 dots wide. `COMDAT.ASM` RAMDAT stores `VCNTRX` = 128 and `VCNTRY` = 76. `CD.ASM` `D0.LEN` is `32*19*8` scan lines, so the graphics viewport ends at line 152. `COMDAT.ASM` `STSVDB` names the 152nd and 160th scan lines; `PRIVDB` names the 160th and 192nd. Status occupies 152–160; the command region occupies 160–192.

## Viewer

**[SRC]** `VIEWER.ASM` walks `RANGE` from 0 through 9. At each cell it unpacks the four two-bit edge fields into `NEIBOR`, indexes them by `PDIR`, and draws `SWCHAR.ASM` `FLATAB` in left, forward, right order. Feature codes are `HF.PAS` 0, `HF.DOR` 1, `HF.SDR` 2, `HF.WAL` 3 (`CD.ASM`). A secret door is drawn with magic light, then replaced by a wall drawn with regular light.

**[SRC]** Draw order in one cell is architecture, then the creature `CFIND` returns, then peek-a-boo (`PDRAW`: a passage on that side and a creature in the stepped-to cell, using `LPEEK`/`RPEEK` from `VARC.ASM`), then `VFIND` (or `CELINE` when none), then unowned objects from `OFIND`. Objects are submitted to `DRAWIT` twice: magic light, then regular light.

**[SRC]** Line of sight stops when the forward edge is not a passage. Doors and secret doors block the walk. After a step, `RANGE` increments; `CMPA #9 / LBLE VIEW10` draws at most ranges 0–9.

**[SRC]** `SETSCL` loads `NORSCL` for a standing look: 200, 128, 80, 50, 31, 20, 12, 8, 4, 2 in 128ths (`VIEWER.ASM`). `HLFSCL` and `BAKSCL` are extracted and unused until half-step turn/move animation.

**[SRC]** `VCTLST.ASM` `SETFAX`: fade 0 when `light - 7 - RANGE >= 0`, fade `$FF` (draw nothing) when that difference is `<= -7`, otherwise `BITMSK+8` indexed by the negative remainder (`VECTOR.ASM` `BITMSK` is `BIT7`…`BIT0`). Regular architecture uses `RLIGHT`. `MAGFLG` selects `MLIGHT` for secret-door marks, magical creatures (`P.CCMGO` nonzero), peeks of those creatures, and the first of the two object draws.

**[SRC]** Vector lists live in `VARC.ASM`, `VERT.ASM`, `VOBJ.ASM`, `D3.ASM`, `D4.ASM`. `V$FF` starts a new pen, `$FE` ends, `$FC` relative nybbles, `$FB`/`$FA` subroutine, `$FD` jump. This projection records logical endpoints, not rasterised pixels.

**[OPEN]** `HLFSTP`/`BAKSTP` half-step frames and `VECTOR` DDA fade-to-pixels are not in this draw list. Map, examine, and text are separate projections below. Sound-loop jiffy durations remain D-4b.

## Core events

**[SRC]** The simulation emits an ordered `CoreEvent` stream (ADR-0004 rule 1): sound requests (`SOUNDS`/`ISOUND`), `OUTSTI` text, stores to `DSPMOD`, foreground blocks (`SYNC`, `TURN`/`MOVE` sweeps, `WAITX`), and the `CLOCK` `CLK30` heartbeat toggle. Each event is stamped with the interrupt count and the running task (`IRQ`, a TCB name, or `FG`). Where D-4a/D-4b apply, `duration_jiffies` is 0 and `duration_known` is false. `dcli --events` prints the stream; the default trace keeps its older `SOUND`/`SYNC` lines.

## Sound

**[SRC]** `SOUNDS.ASM` `SNDTAB` is 23 cues; `SNDOBJ` is 12. Creature type is the cue index. Object classes add `SNDOBJ`. `ISOUND` forces `B = $FF`. `CWLK20` attenuates with `B = ~(T0 * 31)` when the larger row/column delta is ≤ 8 and the smaller ≤ 2. A same-cell attack uses `$FF`. `SNOISE` updates `SNDRND` only and does not read or write `SEED`. Generator immediates and `THUDD`/`BANGD` are in `fixtures/sounds.json`. Jiffy durations remain D-4b.

## Mapper, examine and text

These are character- and cell-level projections. They do not rasterise.

**[SRC]** `MAPPER.ASM`. Each maze cell is one byte wide and six scanlines tall (`DSP32` multiplies the row by `32*6`). A cell whose maze byte is `$FF` is filled solid; any other byte is left empty. The header comment names door colours; the routine does not paint per-edge doors. When `MAPFLG` is set, unowned objects on the level (`FNDOBJ`, skip if `P.OCOWN` is set) and live creatures are marked, then the player, then both `VFTTAB` groups from `VFTPTR` (`BSR MAPP60` and the fall-through). `dcli --present-map` prints that occupancy list.

**[SRC]** `PEXAM.ASM` `EXAMIN`. The examine TXB is the 19×32 viewer (`COMDAT.ASM` `TXTEXA`, `32*19` cells). Cursor starts at 10. The packed strings are `IN THIS ROOM`, `!CREATURE!`, and `BACKPACK`. Floor objects come from `OFIND` at `PROW`/`PCOL`. `PRTOBJ` alternates a 16-column tab (`ADDD #16` / `ANDB #$F0`) with `PCRLF`. A revealed object is adjective plus generic from `ADJTAB`/`GENTAB`; an unrevealed object is generic only (`STATUS.ASM` `OBJNAM`). The burning torch is inverse. `dcli --present-text` prints the examine pad followed by status and command.

**[SRC]** `STATUS.ASM` `STATUX`. Fifteen spaces at columns 0 and 17, left-hand name at 0, right-hand name right-justified from 33 minus the `$FF`-terminated length. Empty hands are `M$EMPT` `EMPTY`. **[SRC]** `COMMON.ASM` `CLK30` deposits the heart at column 15 when `HEARTF` is set (`PLOOK` turns it on, a map scroll clears it). **[SRC]** `MISC.ASM` `PROMPT` writes `I.CR`, `I.DOT`, underline, backspace. `TXTPRI` is four 32-column lines (`COMDAT.ASM`).
