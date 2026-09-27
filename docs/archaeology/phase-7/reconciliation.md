# Phase 7 reconciliation

The offscreen rasterizer follows `VECTOR.ASM`: length is the larger absolute delta, the walk starts half a pixel in, and each step adds the 8.8 increment from `DIVIDE`. The fade counter skips steps, and set pixels pack with `BITMSK`. Absolute and relative vector lists are decoded. `$FB` and `$FD` follow a big-endian index when it falls inside the same buffer; `$FA` returns. An index past the buffer is a 6809 address and decoding stops. `$FF` starts a new pen. `SNOISE`, `SNOUT`, and the noise-pulse loop (`PSSHT`, `PSSST`, `RATTLE`) produce DAC bytes. `jiffies_due` turns host time into a jiffy count and keeps the remainder, so a stall runs the missed jiffies.

SDL3 3.2.16 was built from source on this machine and `dod` linked. The window copies the 3× scaled frame into an RGB texture. It was not opened, because this session has no display.

`kObjects` rows 18–24 are the `SPCXXX` special objects from `docs/archaeology/phase-0b/fixtures/objects.json` (`special_objects`). `INCANT`, `BURNER`, and `USE` store those type indices, and `OCBFIL` indexes the table by type. The placed `OBJXXX` rows stop at 17, so the special rows are what keep those transforms inside the table. `Game::press` stamps each key on its own jiffy so one interrupt still receives at most one new character. `set_fade` returns `0xFF` for a signed level of -7 or below, so `kBitMask[0]` and `kBitMask[1]` are not reached. The tested boundary is light 1 at range 0, which is signed level -6 and mask `0x20`.

## C-17: faint and death fade pacing (2026-09-26)

The capture ran on MAME 0.264 `coco2b` with the retail 26-3093 image, `tools/rom/run-capture.sh`, and an empty keystroke script. It is **harness-modified**: `DOD_POKE=100:PDAM:PPOW+20` writes `PDAM` = 180 at isr 100 (`capture.lua` prints the line). The capture is not in the tree (`captures/c17-death.rom.*`). Observed, from the per-interrupt raw samples:

- `HUPDAT` ran at isr 106 and set `HEARTR` 1. `HUPD30` lowered `MLIGHT`/`RLIGHT` one step per 5 jiffies. `RLIGHT` went from $00 to $F8 between isr 112 and 147.
- `FAINT` went to $FF and `HBEATF` to 0 at isr 148. `WIZIX` started at isr 149 (`VCTFAD` $20, `NOISEF` $FF): 2 jiffies after the last `RLIGHT` step and 1 after `FAINT`. The capture does not record when `ZFLOP` blanks the screen, so the port measures the gap from the last fade-out frame (2 jiffies).
- `VCTFAD` reached each even value about 18 jiffies after the previous one, down to $00 at isr 439. `NOISEF` cleared at isr 461. Odd values also appear between the even steps and sometimes last most of a step (for example $1F from isr 169 to 185). Their cause is **unresolved**, and the port does not use them.
- The explosion followed. `FAINT` cleared at isr 542, after the epitaph.

`NOISEF` stays set for 22 jiffies after `VCTFAD` reaches $00, so the port waits a full step after the last frame as well (17 × 300 ms ≈ 5.1 s, compared with 312 jiffies ≈ 5.2 s from isr 149 to 461). D-14 is presentation-only, so there is no core regression test. `src/platform/sdl_app.cpp` now paces D-14 to these values (83 ms, 33 ms, 300 ms), labelled [ROM]. Before this, the wizard fade took about 1.9 s; on the ROM it takes about 4.8 s. This is only the level-0 view with `RLIGHT` starting at 0. The fade-out has more steps when the start is brighter, one per level of light.
