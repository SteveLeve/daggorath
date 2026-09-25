Carry out **Phase 6: presentation state** for the Dungeons of Daggorath
preservation project. Read the charter (presentation fidelity), `CLAUDE.md`,
`docs/architecture/module-boundaries.md`, ADR-0004 and ADR-0006, the Phase 0
report §§14–15 and the latest reconciliation first. Produce working artifacts,
not a plan. Do not draw pixels or open a window.

**Hypotheses, not evidence.** Behaviour this prompt names (priorities,
thresholds, effects, routine roles) comes from earlier reports and is a target
to verify against the listing. Cite the listing, never this prompt.

**Preservation requirement.** What the player could see and hear is a function
of state the original computed: `VIEWER`'s range walk and edge parsing,
line-of-sight stop, regular versus magical light, draw order of architecture,
creatures, vertical features and objects; `MAPPER` with and without features;
`EXAMIN`; the status and command text regions; `SOUNDS` event selection and
distance volume; the heartbeat toggle. Blocking animations and sounds block the
foreground for as long as the source makes them, where that is knowable.

Work in this order.

1. **Settle ADR-0004's projection half.** `CoreEvent` and the blocking model
   already exist from Phase 6a. Define `RenderState` in `src/presentation/` as a
   pure projection of a read-only core snapshot plus `CoreEvent`s. `src/core`
   includes no presentation header; if the projection needs a core value, core
   exposes the value.
2. **Extract** vector geometry (`VARC`, `VERT`, `VOBJ`, `D3`, `D4`), `VCTLST`
   encoding, scale tables (`NORSCL`, `HLFSCL`, `BAKSCL`), font and text
   positions, and `SOUNDS` parameters into fixtures with provenance, and
   generated headers per ADR-0006.
3. **Specify** `docs/specification/rendering-and-audio.md`: coordinates (256
   wide, centroid (128,76), viewport to line 152, status to 160, command to 192
   per the Phase 0 report — re-verify from the listing), projection, fades,
   the `SNOISE` question (does it consume `SEED`?) answered from the listing,
   and D-4 durations: derive what the listing determines (loop counts), leave
   the rest unresolved.
4. **Implement** the viewer as a draw list of vectors in logical coordinates
   (not pixels), map mode, text regions, and the sound-event stream. Add a
   `dcli` option that prints the projection in stable text.
5. **Tests:** draw-list fixtures for fixed states on every level (Python
   reference implementation independent of the C++), map mode, darkness,
   magical light, same-cell creature sound at maximum volume.
6. **Reconcile** Phase 6; capture backlog gains screenshot and audio captures
   per fixed state.

**Do not build:** rasterisation, SDL, audio output, input adapters.

**Completion gate.** `make all` output; draw-list fixture match counts; which
D-4 durations are now source-derived and which remain unresolved.
