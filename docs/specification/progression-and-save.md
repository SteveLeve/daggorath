# Progression and save

**[SRC]** unless noted. Dialogue strings are the `OUTSTI` bytes decoded by
`tools/decode_outsti.py`; `^` is `I.CR` (carriage return).

## Endings

Killing creature type 10, the wizard's image, runs `ENDGAM` (`PATTK.ASM`). It prints `^ ENOUGH! I TIRE OF THIS PLAY...` and `   PREPARE TO MEET THY DOOM!!!`. `BAGPTR` becomes `PTORCH`, whose link is cleared, so the torch in use (lit or dead) is the only bag object. `PLHAND`, `PRHAND`, and `PTORCH` are kept. `POBJWT` is set to 200. `NEWLVL` builds level 3, which re-runs `SYSTCB`, and `FNDCEL` calls `RNDCEL` until the cell is not `$FF`; `RNDCEL` (`DGNGEN.ASM`) draws the column into `B` first and then the row into `A`.

Killing creature type 11, the wizard, runs the ring riddle. `DEC FRZFLG` freezes creatures. `PRLITE`/`PMLITE` become `$07`/`$13`. `BAGPTR`, `PTORCH`, `PRHAND`, and `PLHAND` are cleared. The creature's own objects, including the supreme ring it carries on level 4, have already dropped to its cell (`PATT30`). **[OPEN]** `OCBPTR` is set to the second object block; the core has no object-allocation pointer, and nothing in Original Mode allocates an object after this point.

`INCANT` checks the left hand and then the right (`PINCAN.ASM`). A held ring whose `P.OCXXX+1` equals the fully spelled token becomes that type. When the new type is `T.RN15` (`FINAL`), `WINNER` prints `^BEHOLD! DESTINY AWAITS THE HAND` and `        OF A NEW WIZARD...`, then halts (`BRA *`).

Death is `PPOW < PDAM` in `HUPDAT` (`BLO`). `DEATH` prints `^ YET ANOTHER DOES NOT RETURN...` and ends in `BRA *`.

## Save and load

`ZSAVE name` and `ZLOAD name` take the next token as an eight-character filename (`PZTAPE.ASM FILNAM`). They only set `ZFLAG`: +1 for save, -1 for load.

`SCHED1` tests `ZFLAG` after each task is requeued, so the tape operation happens once the `PLAYER` task that parsed the command has finished its drain and been requeued. Any keys still buffered behind the command are consumed by that drain first.

| Block | Range | In the image |
|---|---|---|
| Direct page | `DP.BEG` (`$0200`) – `DP.END` | player block, lights, `FAINT`, `BAGPTR`, `FRZFLG`, `SEED`, clock `JIFFY`–`DAY`, queue heads, heart bytes, `DSPMOD`, `ZFLAG`, scheduler bytes, keyboard pointers |
| Common RAM | `MM.BEG` – `MM.END` | `KBDBUF`, `LINBUF`, `TOKEN`, text blocks, `CMXLND`, `CCBLND`, `MAZLND`, `TCBLND`, `EMPHND`, `OCBLND` |
| Not saved | `MM.END` – `$1000` and above | the stack (`PDL`) and the video buffers |

`SAVE` writes 128-byte blocks over both ranges (`COMMON.ASM`). `LOAD` copies them back to `$0200` without calling `NEWLVL`. Both paths end in `LOAD90`: `IRQSYN`, `CLR ZFLAG`, `INIVU` (`HUPDAT` and then `PLOOK`, which selects the viewer), `PROMPT`, and a `SCHED` lap from the head of `SCDQUE` after the next interrupt. `PIATAP` disables the IRQ, so no jiffy is counted and no key is polled during tape I/O.

The core's `ram_image()` holds every modeled field in those ranges. A `ZLOAD` therefore resumes exactly at the state just after the matching `ZSAVE`. The test `progression_regressions` checks this against a run that never left the save point. The file format and the suspend snapshot are in ADR-0005.

D-11: `LOAD` keeps reading tape until a header matches. The in-memory cassette reports `???` for an absent name instead.
