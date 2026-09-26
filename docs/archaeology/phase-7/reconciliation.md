# Phase 7 reconciliation

The offscreen rasterizer follows `VECTOR.ASM`: length is the larger absolute delta, the walk starts half a pixel in, and each step adds the 8.8 increment from `DIVIDE`. Fade and the 1-bit pixel masks are not applied. `SNOISE` is implemented as the 16-bit `SNDRND` update. The fade counter skips steps the way `VECTOR` does. Set pixels are packed with `BITMSK`: bit 7 of a scan-line byte is column 0 of that byte, and a row is 32 bytes. `jiffies_due` turns host time into a jiffy count and keeps the remainder, so a stall runs the missed jiffies.

SDL3 is not installed in this environment, so the `dod` window target is not built. The core tests do not link SDL. Audio synthesis is not implemented. The Phase 5 power-on playthrough script, which this phase would replay, is still unauthored.
