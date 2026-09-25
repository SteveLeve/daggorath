Carry out **Phase 8: the touch input layer** for the Dungeons of Daggorath
preservation project. Read the charter's Mobile UX section and principle 4,
`CLAUDE.md` ("input reaches the simulation as timestamped keystrokes or parsed
commands, never by a side door"), ADR-0007 and the desktop app first.

**Preservation requirement.** Touch is an adaptation layer. Every gesture
produces the keystrokes (or a parsed command whose effect is identical to
typing it) the original player would have typed; the typed command line stays
available and authoritative. Nothing pauses the simulation unless the original
pauses. Timing pressure is preserved: an overlay that takes time to operate
costs game time.

Work in this order.

1. **Interaction inventory**: from `commands-and-parser.md` and the item and
   combat specifications, list every command form a player needs, with its
   typed equivalent. This is the contract the controls must cover.
2. **Adapters** in `src/input/`: gesture → keystroke stream, with the
   keystrokes timestamped at the jiffy they are delivered. Decide and document
   whether a gesture emits the whole line at once (and what that does to the
   32-byte buffer quirk) or paces keystrokes; record the choice as a deviation
   if it differs from a typist.
3. **Prototype on desktop** with mouse-as-touch; evaluate portrait and landscape
   layouts; optional visible command trace.
4. **Tests**: each gesture's keystroke output as fixtures; a replay of a touch
   session produces the same core trace as its keystroke transcript.
5. **Write** `docs/architecture/touch-input.md` with the layouts evaluated and
   the evidence for the chosen default.

**Do not build:** Android or iOS packaging, rule changes, pausing overlays.

**Completion gate.** Coverage table (every command form reachable by touch),
replay-equivalence test output, `make all` output.
