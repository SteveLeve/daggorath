# Rendering and audio

Logical surface is 256 dots wide. The Phase 0 report places the centroid at (128, 76), the viewport through line 152, status through 160, and the command area through 192. Those line numbers are **[inferred]** from that report until a listing pass cites the video-base constants. The facing scale bytes are **[SRC]** `VIEWER.ASM` `NORSCL`: 128, 128, 80, 50, 31, 20, 12, 8, 4, 2 in 128ths.

`project` emits one wall segment per open cell along the facing ray, scaled by `NORSCL`, and emits none when regular plus magic light is zero. Map mode emits `MAP features` or `MAP plain`. This is not a `VCTLST` interpreter.

## Core events

**[SRC]** The simulation emits an ordered `CoreEvent` stream (ADR-0004 rule 1): sound requests (`SOUNDS`/`ISOUND`), `OUTSTI` text, stores to `DSPMOD`, foreground blocks (`SYNC`, `TURN`/`MOVE` sweeps, `WAITX`), and the `CLOCK` `CLK30` heartbeat toggle. Each event is stamped with the interrupt count and the running task (`IRQ`, a TCB name, or `FG`). Where D-4a/D-4b apply, `duration_jiffies` is 0 and `duration_known` is false. `dcli --events` prints the stream; the default trace keeps its older `SOUND`/`SYNC` lines.

## Sound

**[SRC]** `SOUNDS.ASM` `SNDTAB` is 23 cues; `SNDOBJ` is 12. Creature type is the cue index. Object classes add `SNDOBJ`. `ISOUND` forces `B = $FF`. `CWLK20` attenuates with `B = ~(T0 * 31)` when the larger row/column delta is ≤ 8 and the smaller ≤ 2. A same-cell attack uses `$FF`. `SNOISE` updates `SNDRND` only and does not read or write `SEED`. Generator immediates and `THUDD`/`BANGD` are in `fixtures/sounds.json`. Jiffy durations remain D-4b.
