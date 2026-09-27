# ADR-0007 — Original Mode isolation and extension points

**Status:** Accepted, 2026-09-25.

## Context

The charter requires Original Mode to stay canonical and enhanced modes to
attach "at extension points, never by editing Original Mode". No enhanced mode
is planned before Phase 11, but choices in Phases 2–9 can make isolation cheap
or expensive.

## Decision

1. Original Mode exposes no gameplay choice that alters rules: no player-facing
   seed, no map generator choice, no difficulty. Its production construction path
   derives clock, RNG and state exactly as the original does.
   Tests, replay and snapshot restore may inject a complete initial state or a
   deterministic environment, but only through an explicitly non-gameplay
   harness API that no player-facing surface can reach. A conformance test
   proves the normal construction path is still canonical (the same initial
   state as the source's power-on).
2. Presentation and input options (scaling, line thickness, control layout,
   volume) are allowed in any mode because they do not reach the core.
3. The core's variation points, when first needed, are compile-time or
   construction-time policy objects (maze source, spawn schedule, rule tables),
   with the Original implementation the only one linked into Original Mode
   conformance tests.
4. The Original Mode conformance suite must pass unchanged in any build that
   also contains an enhanced mode.
5. Accessibility aids that change timing (slower clock, pause on overlay) are
   enhanced-mode features, not Original Mode settings. **Narrowed
   2026-09-27 (ADR-0009 accepted, Phase 8.2):** the shell's system-menu
   pause and OS backgrounding (ADR-0009 §3, deviation D-16) withhold jiffy
   delivery outside the core and are available in Original Mode too, because
   they change no core state or behaviour (ADR-0009 §4, pause invariance).
   In-play overlays (pickers, the INCANT keyboard) stay enhanced-mode-only:
   this rule still bars them from pausing. That bar is design intent for the
   workstreams that build those overlays (8.4), not a runtime-enforced
   invariant today: no code path yet connects any overlay to
   `Shell::tick()`.

## Consequences

- Phases 2–9 should not add rule-level flags "for later". If a phase finds it
  wants one, it records the need here instead.
- The charter's "avoid pausing gameplay merely because a touch interface is
  open unless historically appropriate" is enforced by rule 5.
