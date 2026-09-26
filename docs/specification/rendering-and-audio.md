# Rendering and audio

Logical surface is 256 dots wide. **[SRC]** `COMDAT.ASM` `VCNTRX`/`VCNTRY` store the centroid at (128, 76). **[SRC]** `COMDAT.ASM` `STSVDB` base is `D0$BAS+152*32` and end is `D0$BAS+160*32`; `PRIVDB` base is `D0$BAS+160*32` and end is `D0$BAS+192*32`. The viewer therefore occupies scanlines 0–151, the status band 152–159, and the command band 160–191. `CD.ASM` `D0.LEN` is `32*19*8`, which is the same 152-line viewer. The facing scale bytes are **[SRC]** `VIEWER.ASM` `NORSCL`: 128, 128, 80, 50, 31, 20, 12, 8, 4, 2 in 128ths.

`project` emits one wall segment per open cell along the facing ray, scaled by `NORSCL`, and emits none when regular plus magic light is zero. Map mode emits `MAP features` or `MAP plain`. This is not a `VCTLST` interpreter.

**[SRC]** `SOUNDS.ASM SNOISE` updates `SNDRND` only. It does not read or write `SEED`.

D-4 durations: the `BURNER` and `HSLOW` countdowns are source-derived (one minute, and the heart-rate delay). Sound loop counts inside `SOUNDS.ASM` are not yet transcribed, so those durations stay unresolved.

## Mapper, examine and text

These are character- and cell-level projections. They do not rasterise.

**[SRC]** `MAPPER.ASM`. Each maze cell is one byte wide and six scanlines tall (`DSP32` multiplies the row by `32*6`). A cell whose maze byte is `$FF` is filled solid; any other byte is left empty. The header comment names door colours; the routine does not paint per-edge doors. When `MAPFLG` is set, unowned objects on the level (`FNDOBJ`, skip if `P.OCOWN` is set) and live creatures are marked, then the player, then both `VFTTAB` groups from `VFTPTR` (`BSR MAPP60` and the fall-through). `dcli --present-map` prints that occupancy list.

**[SRC]** `PEXAM.ASM` `EXAMIN`. The examine TXB is the 19×32 viewer (`COMDAT.ASM` `TXTEXA`, `32*19` cells). Cursor starts at 10. The packed strings are `IN THIS ROOM`, `!CREATURE!`, and `BACKPACK`. Floor objects come from `OFIND` at `PROW`/`PCOL`. `PRTOBJ` alternates a 16-column tab (`ADDD #16` / `ANDB #$F0`) with `PCRLF`. A revealed object is adjective plus generic from `ADJTAB`/`GENTAB`; an unrevealed object is generic only (`STATUS.ASM` `OBJNAM`). The burning torch is inverse. `dcli --present-text` prints the examine pad followed by status and command.

**[SRC]** `STATUS.ASM` `STATUX`. Fifteen spaces at columns 0 and 17, left-hand name at 0, right-hand name right-justified from 33 minus the `$FF`-terminated length. Empty hands are `M$EMPT` `EMPTY`. **[SRC]** `COMMON.ASM` `CLK30` deposits the heart at column 15 when `HEARTF` is set (`PLOOK` turns it on, a map scroll clears it). **[SRC]** `MISC.ASM` `PROMPT` writes `I.CR`, `I.DOT`, underline, backspace. `TXTPRI` is four 32-column lines (`COMDAT.ASM`).
