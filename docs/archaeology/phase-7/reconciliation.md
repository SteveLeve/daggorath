# Phase 7 reconciliation

The offscreen rasterizer plots an integer DDA on a 256×192 buffer. It is not the fractional `VECTOR` accumulator yet. `jiffies_due` turns host time into a jiffy count and keeps the remainder, so a stall runs the missed jiffies.

SDL3 is not installed in this environment, so the `dod` window target is not built. The core tests do not link SDL. Audio synthesis is not implemented. The Phase 5 power-on playthrough script, which this phase would replay, is still unauthored.
