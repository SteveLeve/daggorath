# ADR-0001 — Phase sequence after Phase 1

**Status:** Accepted, 2026-09-25.

## Context

Charter §6 orders the work coarsely: specification, headless core, CLI harness,
renderer and audio, SDL desktop, touch, Android, iOS, mobile UX, enhanced
modes. "Headless core" is most of the game and is too large for one phase or one
agent session. Phase 1's reconciliation (§7) recommends creature movement
without attacks as the next slice.

## Decision

Split the headless core into four gameplay phases, each adding one family of
scheduled tasks or command handlers:

2. creature movement (`CMOVE` minus attack);
3. combat and physiology to death;
4. objects, inventory, torches, magic, vertical travel;
5. progression, endings, save/load.

Then 6 presentation state, 7 desktop SDL, 8 touch, 9 mobile packaging, 10 mobile
UX, 11 enhanced modes, as in the charter. ROM observation is a parallel track
(ADR-0003). The full table is `docs/planning/roadmap.md`.

## Consequences

- A trace divergence introduced in a phase is attributable to that phase's
  task family.
- Phases 2–4 need a rule for behaviour that belongs to a later phase: ADR-0008.
- The headless game is complete, and playable through `dcli`, at the end of
  Phase 5, before any pixel is drawn. That matches charter §6 ("a mobile
  screen should not become the primary environment for debugging").
- Reordering (e.g. objects before combat) is allowed only by a new ADR that
  says what evidence changed.

## Rationale for the order

**Inferred** from the dependency shape in the Phase 0 report §9–16: `CMOVE`
calls the creature attack, so movement precedes combat; `PATTK` consults the
held object, torch state and ring charge, so a minimal object model exists in
Phase 3 (the starting sword and torch) and the rest arrives in Phase 4; both
endings are reached through a kill or `INCANT`, so they close the sequence.
