---
name: fixture-change
description: Regenerate fixtures from the pinned listing, explain every hash change, and record the reason in reconciliation.md before touching the manifest.
disable-model-invocation: true
---

Fixture hashes are the evidence baseline. Never regenerate one just to make a diff pass.

1. `make check-pin`. If the listing is missing, stop and suggest `make sources`.
2. Save the current state: `cp docs/archaeology/phase-0b/fixtures/MANIFEST.json "$TMPDIR/manifest.before.json"`.
3. `make fixtures`. This rewrites `MANIFEST.json` too (in `tools/extract_fixtures.py`).
   Compare it with the saved copy to see which hashes changed.
4. For each changed fixture, diff its content (`git diff -- <fixture>`) and work out
   *why* it changed: extractor fix, new field, listing reinterpretation. Link it
   to the tool or source change that caused it.
5. Draft a dated entry for `docs/archaeology/phase-0b/reconciliation.md` that lists each
   fixture, its old and new hash, and the reason with the listing location. Show it to
   the user and **wait for approval** of the reason.
6. Only after approval: append the entry, then run `make verify` and `make test`.
   Never edit the manifest by hand.
7. If a change has no explanation, revert the fixtures (`git checkout -- docs/archaeology/phase-0b/fixtures`)
   and report it as unresolved.
