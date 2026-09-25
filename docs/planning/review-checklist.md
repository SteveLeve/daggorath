# Phase review checklist

For a reviewing agent (or human) on any phase PR. Report each item as pass,
fail with location, or not applicable. The `evidence-auditor` and
`boundary-checker` agents under `.claude/agents/` cover items 2–6 and 8
mechanically; item 12 is `tools/check_links.py`; run them and merge their findings.

1. **Scope.** Every changed area belongs to the open phase named in `README.md`.
   Nothing from the prompt's "do not build" list is present. Commands out of
   scope still report `UNIMPLEMENTED`.
2. **Labels.** Every new behavioural claim is source-proven (with a listing
   location), ROM-observed (with a capture), inferred, or unresolved.
3. **No invention.** No fixture value, hash, timing or table entry lacks a
   source location and extraction method. Nothing is typed from memory or a port.
4. **Baselines.** Any `MANIFEST.json` or committed trace change has a dated
   reason in the phase reconciliation. No baseline was regenerated to pass.
5. **Deviations.** Every departure is a D-number in `clock-and-scheduler.md` §13
   with the phase or track that retires it; stubs follow ADR-0008.
6. **Quirks.** No original mechanic is fixed silently; new ones are in
   `quirks.md`.
7. **Tests.** Each newly established behaviour has a conformance test; each
   retired deviation changed a test visibly.
8. **Boundaries.** `src/core` includes no platform header and links nothing;
   input arrives only as timestamped keystrokes or parsed commands.
9. **Provenance.** No ROM, capture, manual scan or port source is staged; new
   external sources and generated data files are in the ledger.
10. **Gate evidence.** The PR or reconciliation shows real `make all` output,
    test counts and the first divergence of any mismatch — not an assurance.
11. **ADRs.** A Proposed ADR the phase was meant to settle has a dated
    Resolution; any decision that constrains later phases has an ADR.
12. **Links.** Every relative link in changed Markdown resolves
    (`python3 tools/check_links.py`), and phase ↔ ADR ↔ spec cross-references
    name documents and sections that exist.
13. **Hand-off.** Reconciliation written, forward pointer appended to the
    previous one, capture backlog updated, `README.md` state updated, next
    phase's prompt amended if the evidence moved.
