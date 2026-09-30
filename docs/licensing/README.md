# Licensing

The evidence and rights position is recorded in
[`../provenance/ledger.md`](../provenance/ledger.md); this file tracks the
questions that must be **answered** before anything is distributed.

## Open questions

1. **Scope of the Morgan preservation grant.** It licenses reproduction of *the
   game* on condition that its original, unaltered form is preserved, and asserts
   a belief about rights reversion from Radio Shack rather than supplying the
   underlying contract. It does not resolve ownership of contributors' art and
   sound, the Tandy manual, trademark use, later port copyrights, or commercial
   terms.
2. **The handwritten grant in the assembly repository** is a separate grant to a
   named person concerning that source. It does not appoint downstream recipients
   to relicense the tree.
3. **The 1983 notice names Unified Technologies** while the manual and the grant
   refer to DynaMicro and Tandy. Record the discrepancy for counsel.
4. **Extracted original data.** The decoded lexicon and the creature, object and
   vertical-feature tables are copied original content, not computed behaviour.
   The generated headers listed in [`../../DATA-NOTICE.md`](../../DATA-NOTICE.md)
   (`lexicon_tables.hpp`, `sound_tables.hpp`, `vector_tables.hpp`,
   `text_tables.hpp`) and the hand-transcribed rows in `src/core/population.cpp`
   carry that data into the build. These are the artifacts that need the grant, or specific
   permission, to ship.
5. **Name and branding.** Title, trademark and store presentation are separate
   from the code licence.
6. **An enhanced or extended mode** is by definition not the original unaltered
   form. Its relationship to the grant's condition needs an explicit answer before
   such a mode ships.

## Decisions (2026-09-29)

These decisions are the owner's. They are interim measures and do not answer the
questions above. Qualified legal review is still required before distribution.

| # | Decision | Where it lives |
|---|---|---|
| D1 | The project's own code, tools, tests and docs are MIT-licensed. The licence explicitly excludes original game content. | [`LICENSE`](../../LICENSE) |
| D2 | Original game data stays in the tree but is marked and isolated. `DATA-NOTICE.md` lists every copied artifact. Each generated header opens with a rights notice, which its generator emits. The project claims no licence over that data and relies on the Morgan grant as-is. It does not invent a sub-licence. | [`DATA-NOTICE.md`](../../DATA-NOTICE.md), `tools/gen_*`, `tools/extract_{sounds,text}.py` |
| D3 | Until review, distribution is source repository only. There is no binary or web release, no app-store listing and no monetisation. | this file, `DATA-NOTICE.md` |
| D4 | Visual or convenience refinements beyond the original are opt-in and off by default. Original Mode renders the original look by default. Each refinement is recorded in `docs/specification/clock-and-scheduler.md` §13 or `quirks.md`. Shipped refinements are audited against this rule in a follow-up. | this file |
| D5 | **2026-09-30.** Phase 9 builds are private-device only. The owner, while still working on questions 1, 4 and 5, asked to start Phase 9 so they can test on their own Android devices, and iOS later. This satisfies the Phase 9 precondition for private builds only. CI builds but uploads no APK or IPA. There is no store listing, public download, branding or original artwork. It does not relax D3. | this file, ADR-0012, `.github/workflows/android.yml` |

Notes on the question list, which inform the decisions without settling
anything:

- Q2: `grant_of_license.png` names **Michael Spencer** (ledger §2). It does not
  name any other archivist.
- Q3: record only sourced names and dates. Do not add co-author credits, release
  years or corporate-filing claims until each one has a source location in the
  ledger.
- Q5: describing the project as "a port of *Dungeons of Daggorath*" is
  descriptive. Using the title as the product's name is a trademark question
  for counsel. Never use Tandy, Radio Shack or TRS-80 marks or logos.
- Q1 and Q4: loading data from a ROM the user supplies, or from a separately
  licensed data pack, is a possible later architecture. It has not been adopted.

## Standing rules

- No ROM image, emulator capture, manual scan, or third-party port source enters
  this repository.
- Third-party code is reference-only until its licence is established and recorded
  in the ledger with origin, pinned commit, author, evidence, intended use and
  reviewer.
- Original Mode and any future extended mode stay separated, so the preservation
  condition can be evaluated against the former on its own.
- Qualified legal review before public distribution, branding or monetisation.
- New copied-data artifacts are added to `DATA-NOTICE.md` and ledger §4 in the
  same change.
