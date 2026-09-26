# Specification

Living behavioural specification. These documents are the contract the
implementation is tested against, and they are updated as evidence arrives — in
contrast to the archaeology reports, which are dated and left alone.

Every rule carries one label: **source-proven**, **ROM-observed**, **inferred**, or
**unresolved**. A capture under replacement firmware is noted as
**replacement-firmware-observed** beside the label and does not make a rule
ROM-observed (ADR-0003). No rule is ROM-observed. The measurement is recorded in
[`../provenance/rom-diff.md`](../provenance/rom-diff.md).

| Document | Covers | State |
|---|---|---|
| [`clock-and-scheduler.md`](clock-and-scheduler.md) | interrupt order, counters, countdown queues, dispatch, keyboard path, command dispatch, physiology timing, level transitions, port obligations, deviations | written in Phase 0b; §11 and D-6 updated in Phase 1 |
| [`dungeon-and-rng.md`](dungeon-and-rng.md) | RNG, level seeds, maze construction, doors, vertical features, movement legality | written in Phase 1 from the Phase 0b source reading |
| [`commands-and-parser.md`](commands-and-parser.md) | the 15 verbs, token tables, prefix matching, failure behaviour for MOVE, TURN, and LOOK | written in Phase 1 from the Phase 0b source reading |
| [`creatures.md`](creatures.md) | population, object attachment, regeneration, when a matrix increment is born | written in Phase 1; movement and attacks are not specified as implemented |
| [`quirks.md`](quirks.md) | canonical Original Mode quirks ledger | Phase 0b rows plus Phase 1 rows |
| `combat-and-items.md` | attack, damage, scaling, items, magic, torches | **to write**: after Phase 1 |
| [`rendering-and-audio.md`](rendering-and-audio.md) | vector data, viewer projection, lighting, mapper, examine, text regions, sound events | Phase 6: screen regions source-proven; mapper/examine/text projections |

When a section here graduates from inferred to proven, record the evidence in the
phase's reconciliation document and add a regression test in the same change.
