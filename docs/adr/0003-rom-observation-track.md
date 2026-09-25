# ADR-0003 — ROM observation as a parallel track

**Status:** Accepted, 2026-09-25.

## Context

Phase 1 proved the listing assembles to the catalog 26-3093 bytes but ran no
emulator frame: MAME's `coco` driver needs Color BASIC 1.2 and Extended Color
BASIC 1.1, which were not legally obtained. The Phase 0 report (§22) had said
"only expand to combat after that slice matches the emulator". Holding every
gameplay phase on that blocker would stall the project indefinitely.

## Decision

1. Gameplay phases may advance on **source-proven** evidence. The byte identity
   in `rom-diff.md` makes source-proven claims claims about the shipped code.
2. ROM observation becomes **Track R**, run whenever a legal path to the system
   ROMs exists. Its prompt is `docs/prompts/track-r-rom-observation.md`.
3. Every phase appends the captures it would have wanted to
   `docs/planning/capture-backlog.md`, with the rule it would promote or refute.
4. Nothing is labelled ROM-observed without a capture recorded per
   `archaeology/phase-0b/traces/README.md` §3 and a ledger row for the ROMs and
   emulator used.
5. Timing that cannot be read from the listing (D-4 animation and sound
   durations, lap cost) stays **unresolved** and modelled as documented
   deviations, never as invented numbers.

## Consequences

- Deviation IDs accumulate until Track R runs. That is acceptable and visible.
- When Track R finds a divergence, the fix lands as its own change with a
  reconciliation entry, even if several phases have been built on the
  divergent rule.
- Candidate legal paths, each needing a ledger entry before use: a dump the
  owner takes from their own CoCo's system ROMs; a licensed distribution; an
  open-source replacement firmware the cartridge runs under (behaviour under
  replacement firmware must itself be validated before its captures count).
