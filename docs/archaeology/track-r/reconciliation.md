# Track R reconciliation

ADR-0003. Captures not tied to an open phase land here (fallback target named
in `docs/prompts/track-r-rom-observation.md` §3). Ledger: firmware and
emulator are `docs/provenance/ledger.md` §1 (MAME 0.264 `coco2b`, owner's
Color BASIC 1.3 / Extended Color BASIC 1.1, hashes matched — authenticated
original system ROMs, not replacement firmware).

## C-09 — first creature move after level entry

**Rule at stake:** `docs/specification/creatures.md` §3: "`CBIRTH` then
allocates a task control block for `CMOVE` on `Q.TEN` with the movement delay
(`COMCRE.ASM QUEADD`). The core does that." Source-proven, not previously
ROM-observed.

**Capture used:** the existing C-01 ROM capture of `t2-forward-corridor`
(`captures/t2-forward-corridor.rom.trace`, MAME 0.264 `coco2b`, owner's
BASIC 1.3/Ext 1.1). No new script was needed: `t2` already runs long enough
into level 0 for the first creature `CMOVE` dispatch to appear, and the Phase
2 reconciliation had already flagged the reference trace's own first
creature-queue line (jiffy 85, `CMOVE-6`) without ever comparing it to the
ROM. This capture pairs that existing ROM trace with a freshly regenerated
core trace (`dcli --script t2-forward-corridor.script --jiffies 200`, no
`--second`, matching `make traces`).

**Alignment problem.** C-21 (start-up alignment, still uncaptured/unresolved)
means the two traces' `PLAYER` dispatches do not share a jiffy origin: the
core's first `PLAYER` run is jiffy 1, the ROM's is jiffy 13 — a 12-jiffy
offset that makes a literal line-by-line diff meaningless past the first
event (`trace_diff.py --relative-to-init` only drops the clock column; it
does not re-align jiffy numbers). This capture instead anchors on the
script's third MOVE-producing command — `M B` (MOVE BACKWARD), the last
keystroke pair in `t2-forward-corridor.script` — which both traces log
explicitly, and measures creature-dispatch timing relative to that shared
local event.

**Observation.**

| | MOVE BACKWARD | `EXERT` after it | first `CMOVE` dispatch | offset (MOVE → first CMOVE) |
|---|---|---|---|---|
| Core (`dcli`) | jiffy 79 | jiffy 79, damage=20 | jiffy 85 | 6 |
| ROM (`coco2b`) | jiffy 84 | jiffy 88, damage=19 | jiffy 90 | 6 |

The first creature to move fires at the same local offset (6 jiffies after
the MOVE BACKWARD command) in both traces. That part of the rule is now
**ROM-observed**.

**Divergence: simultaneity of same-batch births.** The core dispatches nine
creatures (`CMOVE-6` through `CMOVE-14`) on the *same* jiffy, both for the
first birth-driven wave (all nine at jiffy 85) and the next one (all nine
again at jiffy 133):

```
core:  9 @ jiffy 85, 9 @ jiffy 133, 4 @ 169, 9 @ 175, 2 @ 181
```

The ROM capture shows no batch-wide simultaneity. Over the same window it
dispatches individual `CMOVE` tasks at:

```
rom:   90, 91, 91, 96, 102, 108, 114, 120, 126, ... 156, 162, 168, 174, 180
```

— nine dispatches from jiffy 90 to 126, one every ~6 jiffies (one `Q.TEN`
tick), with one coincidental pair sharing jiffy 91, rather than all nine
landing together as the core's model produces. The `TASK run CMOVE` line in the
ROM trace carries no slot number, so which specific creature each dispatch
belongs to is not recoverable from this capture, but the count (9) and the
window match the core's first birth batch closely enough that this is very
likely the same population (level 0 has nine spiders and nine vipers per
`fixtures/creatures.json`; either type's count is 9).

**Reading, not yet a fix.** The current core model treats `NEWLVL`/`CBIRTH` as
instantaneous: every same-type creature gets the identical `Q.TEN` delay
counted from the identical reference jiffy, so they all become ready on the
same tick. The ROM data instead looks like each successive same-type birth
during `CBIRTH`'s loop is queued about one `Q.TEN` tick later than the one
before it — consistent with `CBIRTH` (and the `FNDCEL`/`CFIND` retry loop it
runs per creature) costing real jiffies as it iterates the matrix count,
rather than running to completion within the entry jiffy. This is a reading
of the raw data, not a confirmed mechanism; it needs its own capture (slot
numbers on `CMOVE`, and a jiffy-by-jiffy trace of `CBIRTH`'s own loop) before
any core change. Filed as #35, spec-first per
`docs/prompts/track-r-rom-observation.md` step 5. No core change in this
commit.

**Labels.** `docs/specification/creatures.md` §3's claim that `CBIRTH` queues
`CMOVE` with the type's movement delay is now ROM-observed for the *first*
dispatch's timing. The claim that a same-type birth batch becomes ready
simultaneously is not promoted — it is the open question in the divergence
above, and stays labelled as a core deviation pending the follow-up capture.

**Backlog.** `docs/planning/capture-backlog.md` C-09 row updated to
`captured — track-r reconciliation, first dispatch timing matches; batch
simultaneity diverges (issue #35)`.

**Gate.** No ROM or capture staged: `t2-forward-corridor.rom.trace` predates
this session (C-01, 2026-09-25) and lives outside the tree under gitignored
`captures/` — this session captured nothing new, so nothing new needs
staging. `git status --porcelain` shows only the doc/backlog changes in this
commit.
