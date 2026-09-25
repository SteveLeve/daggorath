# Specification

Living behavioural specification. These documents are the contract the
implementation is tested against, and they are updated as evidence arrives — in
contrast to the archaeology reports, which are dated and left alone.

Every rule carries one label: **source-proven**, **ROM-observed**, **inferred**, or
**unresolved**. As of Phase 0b no rule is ROM-observed; see
[`../provenance/ledger.md`](../provenance/ledger.md) §5.

| Document | Covers | State |
|---|---|---|
| [`clock-and-scheduler.md`](clock-and-scheduler.md) | interrupt order, counters, countdown queues, dispatch, keyboard path, command dispatch, physiology timing, level transitions, port obligations, deviations | written in Phase 0b |
| `dungeon-and-rng.md` | RNG, level seeds, maze construction, doors, vertical features, movement legality | **to write**: the behaviour is proven and fixtured, but lives only in `reconciliation.md` §1.2 |
| `commands-and-parser.md` | the 15 verbs, token tables, prefix matching, failure behaviour per command | **to write**: proven in `reconciliation.md` §1.3 and fixtured |
| `creatures.md` | population, movement priorities, regeneration, creature tasks | **to write**: Phase 1 |
| `combat-and-items.md` | attack, damage, scaling, items, magic, torches | **to write**: after Phase 1 |
| `rendering-and-audio.md` | vector data, viewer projection, lighting, sound events | **to write**: later |

When a section here graduates from inferred to proven, record the evidence in the
phase's reconciliation document and add a regression test in the same change.
