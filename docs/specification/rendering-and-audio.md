# Rendering and audio

Logical surface is 256 dots wide. The Phase 0 report places the centroid at (128, 76), the viewport through line 152, status through 160, and the command area through 192. Those line numbers are **[inferred]** from that report until a listing pass cites the video-base constants. The facing scale bytes are **[SRC]** `VIEWER.ASM` `NORSCL`: 128, 128, 80, 50, 31, 20, 12, 8, 4, 2 in 128ths.

`project` emits one wall segment per open cell along the facing ray, scaled by `NORSCL`, and emits none when regular plus magic light is zero. Map mode emits `MAP features` or `MAP plain`. This is not a `VCTLST` interpreter.

**[SRC]** `SOUNDS.ASM SNOISE` updates `SNDRND` only. It does not read or write `SEED`.

D-4 durations: the `BURNER` and `HSLOW` countdowns are source-derived (one minute, and the heart-rate delay). Sound loop counts inside `SOUNDS.ASM` are not yet transcribed, so those durations stay unresolved.
