Apply the owner's decision on **issue #13** and close Phase 1 for the
Dungeons of Daggorath preservation project, on branch
`cursor/phase-1-level-population` (PR #1, tracking issue #2). Read
`CLAUDE.md`, `docs/project-instructions.md`,
`docs/specification/clock-and-scheduler.md` §13,
`docs/archaeology/phase-1/reconciliation.md` §1 and §6, and the latest
comment on issue #13 before changing anything. Produce the applied decision,
not a plan. This prompt continues `phase-1-rom-captures.md`. It does not
replace the preservation requirement in `phase-1-conformance-and-creatures.md`,
which still overrides anything here.

**Where things stand (commit f669183).** The cartridge matches catalog
26-3093. The ROM captures are in reconciliation §1. The literal first
divergence of `t1`–`t5` is still jiffy 0: reference `INIT` `0:0:1.0.0` versus
ROM `0:0:6.2.5`. During the level-0 build the ROM runs `DGEN90` at
`SECOND` = 6 (6 draws; seeds `3ACBDC` → `8FC8AD` → `0766CB`) and the opening
`CREGEN` increments type 5. The current core, given `SECOND` = 6, matches
those seeds and the 24 positions. `population-entry.txt`
(`cregen 24 24 25 9 25`) is the source-derived `SECOND` = 1 case. The core
was not changed. Do not re-derive the captures, the firmware, or the byte
identity.

**Stop if the owner has not decided.** Issue #13 must contain an owner
decision in a later comment than the 2026-09-25 evidence comment: either
count the 377 build interrupts, or keep `SECOND` = 1 as a documented
deviation. A recommendation is not a decision. If that comment is absent,
say so once, change nothing, and stop. Do not pick a side.

**If the decision is to count the 377 interrupts:**

- The scheduler entry that the reference calls jiffy 0 must show clock
  `0:0:6.2.5` and `SECOND` = 6, because that is what the ROM samples after
  377 interrupts from `GAME10`'s `IRQSYN` to `GAME50`.
- Level-0 `DGEN90` must draw 6 times from entry seed `3ACBDC`. The opening
  `CREGEN` must increment type 5, leave 24 creatures alive, and the next
  `NEWLVL` for level 0 must birth 25.
- Before any fixture, stored trace, or `MANIFEST.json` changes, write the
  reason in `docs/archaeology/phase-1/reconciliation.md`. Never regenerate a
  baseline to make a diff pass.
- Record the new initial clock in `clock-and-scheduler.md` §13. Retire the
  "core still starts at `0:0:1.0.0`" sentence in reconciliation §6.
- `population-entry.txt`'s `SECOND` = 1 rows may stay as a source-derived
  comparison. The ROM-observed level-0 entry is `SECOND` = 6. Say which
  file is which.

**If the decision is to keep `SECOND` = 1:**

- Do not change the core, a fixture, or a stored trace.
- Add the deviation to `clock-and-scheduler.md` §13: the reference starts at
  `0:0:1.0.0` and `DGEN90` sees `SECOND` = 1, while the ROM build leaves
  `0:0:6.2.5` and `SECOND` = 6. Point at reconciliation §1 for the numbers.
- Leave `population-entry.txt` as the source-derived `SECOND` = 1 case.
- Say in reconciliation §6 that this is a documented deviation, not an
  unmeasured gap.

**Either way:**

- Update issue #13 and issue #2's checkboxes to match the decision.
- Update PR #1's description. Mark the pull request ready only after `make
  all` exits 0 and reconciliation §6 no longer says the decision is waiting.
- Run `make all` and report the actual output: ctest counts, `make verify`,
  and any first divergence. A fixture or trace change must show up as a
  recorded regeneration, with the first divergence of the old baseline stated
  before the new files are treated as the baseline.
- `git status --porcelain` must show no ROM, firmware, or capture file.

**Do not.** Re-open the five-minute `CREGEN` reschedule (it is unresolved in
§5; one capture already showed it did not run). Do not implement `CMOVE`,
attacks, or the `PATT40` decrement. Do not retire D-6. Do not "fix" D-4 by
making animation cost the measured jiffies unless the owner asks; D-4 stays
a deviation whose durations are now known. Do not treat the harness-modified
`SECOND` = 0, 1, 7, 30, 59 spins as unmodified ROM behaviour.
