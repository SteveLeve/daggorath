# Phase 7 reconciliation

The offscreen rasterizer follows `VECTOR.ASM`: length is the larger absolute delta, the walk starts half a pixel in, and each step adds the 8.8 increment from `DIVIDE`. The fade counter skips steps, and set pixels pack with `BITMSK`. Absolute and relative vector lists are decoded. `$FF` starts a new pen. JSR and JMP still use 16-bit memory addresses and are not decoded from a flat list. `SNOISE`, `SNOUT`, and the noise-pulse loop (`PSSHT`, `PSSST`, `RATTLE`) produce DAC bytes. `jiffies_due` turns host time into a jiffy count and keeps the remainder, so a stall runs the missed jiffies.

SDL3 3.2.16 was built from source on this machine and `dod` linked. The window copies the 3× scaled frame into an RGB texture. It was not opened, because this session has no display.
