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
   The generated `src/core/include/daggorath/lexicon_tables.hpp` carries that data
   into the build. These are the artifacts that need the grant, or specific
   permission, to ship.
5. **Name and branding.** Title, trademark and store presentation are separate
   from the code licence.
6. **An enhanced or extended mode** is by definition not the original unaltered
   form. Its relationship to the grant's condition needs an explicit answer before
   such a mode ships.

## Standing rules

- No ROM image, emulator capture, manual scan, or third-party port source enters
  this repository.
- Third-party code is reference-only until its licence is established and recorded
  in the ledger with origin, pinned commit, author, evidence, intended use and
  reviewer.
- Original Mode and any future extended mode stay separated, so the preservation
  condition can be evaluated against the former on its own.
- Qualified legal review before public distribution, branding or monetisation.
