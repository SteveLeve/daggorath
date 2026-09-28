Carry out **Phase 8: the touch input layer** for the Dungeons of Daggorath
preservation project. Read the charter's Mobile UX section and principle 4,
`CLAUDE.md` ("input reaches the simulation as timestamped keystrokes or parsed
commands, never by a side door"), ADR-0007 and the desktop app first.

**Hypotheses, not evidence.** Behaviour this prompt names (priorities,
thresholds, effects, routine roles) comes from earlier reports and is a target
to verify against the listing. Cite the listing, never this prompt.

**Preservation requirement.** Touch is an adaptation layer. Every gesture
produces the keystrokes (or a parsed command whose effect is identical to
typing it) the original player would have typed; the typed command line stays
available and authoritative. Nothing in play pauses the simulation: an overlay that
takes time to operate costs game time. The one pause is the shell's system
menu (and OS backgrounding), which withholds jiffies outside the core
(ADR-0009, deviation D-16).

Read [`docs/planning/phase-8-plan.md`](../planning/phase-8-plan.md), ADR-0009
and ADR-0010 before starting; the plan's workstreams 8.0–8.5 set the order.

Work in this order.

1. **Interaction inventory**: from `commands-and-parser.md` and the item and
   combat specifications, list every command form a player needs, with its
   typed equivalent. This is the contract the controls must cover.
2. **Adapters** in `src/input/`: gesture → keystroke stream, with the
   keystrokes timestamped at the jiffy they are delivered. A gesture emits its
   whole line on one jiffy (decided; deviation D-17). Show that the 32-byte
   buffer overrun stays reachable by typing.
3. **Prototype on desktop** following the agreed direction in
   `docs/design/touch-controls/README.md` (landscape overlay, square letter
   buttons, sequential command entry), with mouse-as-touch; evaluate landscape
   layouts; optional visible command trace.
4. **Tests**: each gesture's keystroke output as fixtures; a replay of a touch
   session produces the same core trace as its keystroke transcript.
5. **Shell** (ADR-0009): pause/resume, system menu, snapshot slots, Restart
   through the canonical constructor; a pause-invariance test.
6. **Render styles** (ADR-0010): `crisp` default drawn from the same draw
   list as `pixel`; segment fixtures; golden images unchanged.
7. **Write** `docs/architecture/touch-input.md` with the layouts evaluated and
   the evidence for the chosen default.

**Do not build:** Android or iOS packaging, rule changes, in-play pausing
overlays, any shell action beyond ADR-0009 §2.

**Completion gate.** Coverage table (every command form reachable by touch),
replay-equivalence and pause-invariance test output, `crisp` segment fixtures,
`make all` output.
