# Track R reconciliation

ADR-0003. Captures not tied to an open phase land here (fallback target named
in `docs/prompts/track-r-rom-observation.md` §3). Ledger: firmware and
emulator are `docs/provenance/ledger.md` §1 (MAME 0.264 `coco2b`, owner's
Color BASIC 1.3 / Extended Color BASIC 1.1, hashes matched — authenticated
original system ROMs, not replacement firmware).

## C-10 — two creatures and a keystroke expiring in one jiffy (attempted, blocked)

**Rule at stake:** `docs/planning/capture-backlog.md` C-10: ADR-0002's lap
policy — "jiffy queue (`PLAYER`) before `Q.TEN` (`CMOVE`)" — when a keystroke
completion and multiple `Q.TEN` expirations land on the same jiffy.

**Attempt.** C-09's ROM capture of `t2-forward-corridor` (`captures/
t2-forward-corridor.rom.trace`) showed the first creature birth batch (nine
`CMOVE` dispatches) landing together at core jiffy 85. A new script,
`scratch/c10-tie.script` (`82 L` / `84 CR`), was designed against the core
model: typing `L` at jiffy 82 and `CR` at jiffy 84 makes the core dispatch
`LINE "L"` — the keystroke completion — on jiffy 85, the same jiffy as the
predicted creature batch. This was verified against the core first: `dcli`
confirms the tie exists in the core trace — `PLAYER`, `LINE "L"`, and nine
`CMOVE-6`..`CMOVE-14` dispatches all fall on jiffy 85 there.

**Run 1 result — no tie.** The same script through MAME 0.264 `coco2b` did
not reproduce it. ROM `CMOVE` dispatches: `78, 91, 91, 96, 102, 108, 114,
120, 126, ..., 156, 162, 168, 174, 180` (`captures/c10-tie.rom.trace`). From
jiffy 102 onward this is identical, jiffy for jiffy, to the tail C-09 already
recorded for `t2-forward-corridor` (`102, 108, 114, 120, 126, ..., 156, 162,
168, 174, 180`) — despite this script having no `MOVE` commands at all; the
two traces' clusters differ before that point (`t2-forward-corridor` starts
its cluster at jiffy 90 with three tightly-packed dispatches, `90, 91, 91`;
this run starts at 78 with a lone dispatch, then `91, 91`). `LINE "L"` fired
alone at jiffy 84, 6 jiffies after the single dispatch at 78 and 7 jiffies
before the paired dispatch at 91 — not a tie either side.

**Run 2 — retargeted, still no tie, and the target moved.** A second script
(`scratch/c10-tie2.script`, `89 L` / `91 CR`) aimed the keystroke at the
pair observed in run 1 (jiffy 91). This run's `CMOVE` dispatches were
`78, 84, 90, 98, 102, 108, 114, 120, 126, ...` — identical to run 1 from
jiffy 102 onward, but the middle stretch that was `91, 91, 96` in run 1 came
out as `84, 90, 98` here: three still-separate dispatches, none landing on
jiffy 91, where `LINE "L"` fired alone instead (`captures/c10-tie2.rom.trace`).

**Reading, not yet a fix.** Two independent, uncorrelated-looking results:
first, the core's premise for this whole experiment — that creature dispatch
jiffies are a fixed function of level entry, independent of later player
input — is the thing C-09 already found false in aggregate, and these two
captures corroborate that at the level of an individual script (the tail
sequence shifted between run 1 and run 2, so it isn't simply "the same fixed
schedule, offset from a different anchor"). Second, and more specifically:
moving the keystroke's jiffy from 84 to 91 changed which nearby jiffies the
surrounding creatures dispatched on (the pair at 91 in run 1 became singles
at 84/90/98 in run 2), while jiffies two or more `Q.TEN` ticks away (102
onward) were unaffected in both runs. That is consistent with the keystroke
and nearby creature dispatches competing for something jiffy-local — closer
to ADR-0002's "one bounded lap per jiffy" than the core's current model,
which never lets a keystroke and a creature's readiness interact at all.

Both readings are plausible from this data; neither is confirmed. **The
practical consequence for C-10 specifically:** a tie cannot be engineered by
predicting a ROM jiffy from the core model and aiming a keystroke at it,
because the act of aiming shifts the target. Capturing the intended tie
needs either a Q.TEN-state watch (so the script can react to the ROM's own
countdown rather than a precomputed guess) or many more trial runs to map
the coupling directly — out of scope for this capture.

**Labels.** No label promoted; C-10's rule stays unresolved. Filed as #43
(the keystroke/creature-dispatch coupling implied by run 1 vs run 2, which
may share a root cause with C-11's task-dispatch-frequency divergence, #39)
per `docs/prompts/track-r-rom-observation.md` step 5. No core change in this
commit.

**Backlog.** `docs/planning/capture-backlog.md` C-10 row updated to
`attempted, blocked — track-r reconciliation: core-predicted ties don't
reproduce in the ROM and the target moves under a retargeted keystroke
(issue #43)`, not `captured`: the rule at stake (dispatch
order under a genuine three-way tie) was not actually observed in either
run.

**Gate.** `captures/c10-tie.rom.trace` and `captures/c10-tie2.rom.trace` are
new but live outside the tree under gitignored `captures/`; the two
`.script` files used to produce them are scratch files under `scratch/`, not
committed (the capture is reproducible from the jiffy numbers quoted above).
`git status --porcelain` shows only the doc/backlog changes in this commit.

C-09 and C-11 also append to this file, in PRs #36 and #41 respectively,
opened before this one and not yet merged as of this commit. Whichever of
the three merges last will need a small rebase to combine all three
sections into one file; that's expected and not a sign anything here is
wrong.
