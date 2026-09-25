Carry out **Phase 7: the desktop SDL3 application** for the Dungeons of
Daggorath preservation project. Read the charter, `CLAUDE.md`,
`docs/architecture/module-boundaries.md`, ADR-0004, ADR-0006, ADR-0007,
`docs/specification/rendering-and-audio.md` and
`docs/specification/clock-and-scheduler.md` §12 first. Produce a working
application, not a plan.

**Hypotheses, not evidence.** Behaviour this prompt names (priorities,
thresholds, effects, routine roles) comes from earlier reports and is a target
to verify against the listing. Cite the listing, never this prompt.

**Preservation requirement.** The desktop app is a presentation of Original
Mode, not a variant. It paces the core at 60 jiffies per simulated second and
never skips a jiffy (§12 obligation 1); under host stalls it catches up by
running every missed jiffy, and records the stall rather than dropping time.
Keyboard input reaches the core as timestamped keystrokes. Lines are drawn as
the original's DDA draws them onto a 256×192 logical surface, then scaled;
sound is synthesised from the `SOUNDS` parameters, not sampled.

Work in this order.

1. **Platform layer** in `src/platform/`: SDL3 window, audio device, timing,
   keyboard. `src/core` gains no dependency (boundary-checker agent must pass).
2. **Rasteriser** in `src/presentation/`: DDA faithful to `VECTOR`, fades,
   text, integer scaling by default; smoothing only as an opt-in presentation
   setting (ADR-0007 rule 2).
3. **Audio**: DAC waveform generators and heartbeat toggle at their event
   times.
4. **Headless golden tests**: render fixed states offscreen to 256×192 buffers
   and hash them; hash audio buffers per sound event. Hashes come from running
   the independent Python references from Phase 6 where possible, never from
   the C++ output alone.
5. **CI**: the core tests stay runnable without SDL.
6. **Record** every platform deviation (frame pacing, audio latency) in
   `clock-and-scheduler.md` §13.

**Do not build:** touch input, mobile packaging, enhanced modes.

**Completion gate.** The app plays the Phase 5 playthrough script unattended
and the resulting core trace is byte-identical to `dcli`'s; golden image and
audio hash results; boundary-checker report.
