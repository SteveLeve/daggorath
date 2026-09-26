# Phase 7 reconciliation

The offscreen rasterizer follows `VECTOR.ASM`: length is the larger absolute delta, the walk starts half a pixel in, and each step adds the 8.8 increment from `DIVIDE`. The fade counter skips steps, and set pixels pack with `BITMSK`. Absolute and relative vector lists are decoded. `$FB` and `$FD` follow a big-endian index when it falls inside the same buffer; `$FA` returns. An index past the buffer is a 6809 address and decoding stops. `$FF` starts a new pen. `SNOISE`, `SNOUT`, and the noise-pulse loop (`PSSHT`, `PSSST`, `RATTLE`) produce DAC bytes. `jiffies_due` turns host time into a jiffy count and keeps the remainder, so a stall runs the missed jiffies.

SDL3 3.2.16 was built from source on this machine and `dod` linked. The window copies the 3× scaled frame into an RGB texture. It was not opened, because this session has no display.

`kObjects` rows 18–24 are the `SPCXXX` special objects from `docs/archaeology/phase-0b/fixtures/objects.json` (`special_objects`). `INCANT`, `BURNER`, and `USE` store those type indices, and `OCBFIL` indexes the table by type. The placed `OBJXXX` rows stop at 17, so the special rows are what keep those transforms inside the table. `Game::press` stamps each key on its own jiffy so one interrupt still receives at most one new character.
