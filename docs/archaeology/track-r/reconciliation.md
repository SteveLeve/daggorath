# Track R reconciliation

ADR-0003. Captures not tied to an open phase land here (fallback target named
in `docs/prompts/track-r-rom-observation.md` §3). Ledger: firmware and
emulator are `docs/provenance/ledger.md` §1 (MAME 0.264 `coco2b`, owner's
Color BASIC 1.3 / Extended Color BASIC 1.1, hashes matched — authenticated
original system ROMs, not replacement firmware).

## C-11 — 10-minute idle run on level 0

**Rule at stake:** `docs/planning/capture-backlog.md` C-11: `CMOVE` priorities
in aggregate, and, in practice, whether `PLAYER` re-queues and dispatches on
every jiffy while idle (the core's assumption; see `docs/specification/
clock-and-scheduler.md` for the jiffy-queue model). Source trace:
`docs/archaeology/phase-2/traces/idle-10min.script` (empty script — no
keystrokes — with `--jiffies 36000`, i.e. 10 simulated minutes on level 0).

**Capture.** New ROM capture, MAME 0.264 `coco2b`, owner's authenticated
BASIC 1.3/Ext 1.1 (`docs/provenance/ledger.md` §1):

```
DOD_JIFFIES=36000 DOD_SECONDS=700 tools/rom/run-capture.sh \
    docs/archaeology/phase-2/traces/idle-10min.script
```

Ran at ~13x real time (MAME reported "Average speed: 1297.97% (609 seconds)").
Output: `captures/idle-10min.rom.trace` (35041 lines, gitignored, not staged).
Compared against a freshly regenerated core trace of the same script
(`dcli --script docs/archaeology/phase-2/traces/idle-10min.script --jiffies
36000`, matching the existing `docs/archaeology/phase-2/traces/idle-10min.trace`,
which is unchanged and was used directly).

As in C-09, C-21 (start-up alignment, unresolved) means the two traces do not
share a jiffy origin for a literal line-by-line diff, so this capture compares
**aggregate dispatch counts over the full 10-minute window** rather than a
per-line trace_diff, which sidesteps the offset entirely.

**Observation — task entries logged over the run:**

| Task | Core (`dcli`) | ROM (`coco2b`) |
|---|---|---|
| `PLAYER` | 36000 (every jiffy) | 28444 |
| `CMOVE` (any creature) | 7591 | 3420 |
| `HSLOW` | 783 | 1307 |
| `LUKNEW` | 2000 | 798 |
| `BURNER` | 11 | 10 |
| `CREGEN` | 3 | 2 |

`BURNER` and `CREGEN` — the minute- and five-minute-scale tasks least
entangled with per-jiffy dispatch order — are close (11 vs 10, 3 vs 2; the
one-count-short readings are consistent with the window's exact start/end
falling a few jiffies either side of a re-queue, not a modelling divergence).
`PLAYER`, `CMOVE`, `HSLOW`, and `LUKNEW` diverge substantially.

**Divergence: the capture logs fewer `PLAYER` entries than the core trace.**
The core logs `PLAYER` in all 36000 jiffies of this idle script — with no
keystrokes, `PLAYER` re-queues on `Q.JIF` and runs again every single modeled
jiffy. The ROM capture's `TASK` log contains `PLAYER` in only 28444 of the
35990 jiffies between its first and last logged `TASK` line (jiffy 11 to
36000) — about 79%.

Looking at *any* task entry, not just `PLAYER`: the ROM trace has a `TASK`
line in 29762 of those 35990 jiffies. **6228 jiffies (17%) show no `TASK`
line.** The remaining gap between "any task" (29762) and `PLAYER` alone
(28444) — about 1318 jiffies — are jiffies where the log has an entry for
some other task (`CMOVE`, `HSLOW`, `LUKNEW`, `CREGEN`, `BURNER`) but no
`PLAYER` entry; 3948 jiffies had more than one logged task dispatch, so these
aren't mutually exclusive categories.

**Reading, not yet a fix.** The core models `PLAYER` as always re-queuing and
running on `Q.JIF` every modeled jiffy (consistent with `PLAYER` counting
36000/36000). An at-most-one-dispatch-per-jiffy explanation is ruled out:
3948 jiffies have multiple logged task entries, and source-proven `SCHED` is
an endless loop, not a bounded per-jiffy pass
(`docs/specification/clock-and-scheduler.md` §5). What remains unresolved is
whether foreground CPU/lap cost — how much scheduler and task work fits between
interrupts — or incomplete `SCHED_JSR` read-tap coverage
(`tools/rom/capture.lua`, which logs the routine at `P.TCRTN`) accounts for the
missing `TASK` entries. These aggregate counts do not establish which
explanation is correct.

This is a reading of the logged counts, not a confirmed mechanism, and it is
larger and less clean than C-09's finding. It needs its own capture (verify
tap coverage and watch `Q.JIF`'s head pointer each jiffy, not just the
dispatch PC) before any spec or core change. Filed as #39, spec-first per
`docs/prompts/track-r-rom-observation.md` step 5. No core change in this
commit.

**Labels.** No label promoted. `BURNER`/`CREGEN`'s close counts are corroborating
but not by themselves enough to promote a specific listing claim to
ROM-observed at this resolution (aggregate counts, not per-event
verification). The `PLAYER`-every-jiffy claim is not promoted and instead
flagged as the open question in the divergence above.

**Backlog.** `docs/planning/capture-backlog.md` C-11 row updated to
`captured — track-r reconciliation, aggregate dispatch counts diverge
(issue #39)`.

**Gate.** `captures/idle-10min.rom.trace` and its companion `.raw.tsv`/task
logs are new but live outside the tree under gitignored `captures/`.
`git status --porcelain` shows only the doc/backlog changes in this commit.
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
does not re-align jiffy numbers). The existing comparison instead anchors on
the script's third MOVE-producing command — `M B` (MOVE BACKWARD), the last
keystroke pair in `t2-forward-corridor.script` — which both traces log
explicitly. This is not a valid anchor for `CBIRTH` queue timing: the creature task was already queued
before the command, the ROM spends foreground time in `LINE`/`MOVE` at jiffy 84
and reaches `EXERT` only at 88, while the core completes the command at 79.
Further, the harness logs task dispatch, not when a countdown becomes ready.

**Observation.**

| | MOVE BACKWARD | `EXERT` after it | first `CMOVE` dispatch |
|---|---|---|---|
| Core (`dcli`) | jiffy 79 | jiffy 79, damage=20 | jiffy 85 |
| ROM (`coco2b`) | jiffy 84 | jiffy 88, damage=19 | jiffy 90 |

These are raw dispatch observations only. The six-jiffy difference between the
MOVE command and first `CMOVE` in each trace does not validate the creature
queue delay. C-09 remains **partial**: the first `CMOVE` dispatch timing relative
to `CBIRTH` is unresolved pending relevant TCB/`QUEADD` timing evidence or
resolution of C-21's start-up alignment.

**Suspected divergence: apparent grouping of birth dispatches.** The core
dispatches nine creatures (`CMOVE-6` through `CMOVE-14`) on the *same* jiffy,
both for the first birth-driven wave (all nine at jiffy 85) and the next one
(all nine again at jiffy 133):

```
core:  9 @ jiffy 85, 9 @ jiffy 133, 4 @ 169, 9 @ 175, 2 @ 181
```

The ROM trace contains individual `CMOVE` dispatches at:

```
rom:   90, 91, 91, 96, 102, 108, 114, 120, 126, ... 156, 162, 168, 174, 180
```

— nine dispatches from jiffy 90 to 126, mostly spaced by one `Q.TEN` tick,
with a pair sharing jiffy 91. This capture cannot establish that these
dispatches belong to one same-type birth batch: `TASK run CMOVE` has no slot or
type identity, and level 0 has multiple populated types, including two with a
count of nine (`fixtures/creatures.json`). The matching count is only an
inference; batch simultaneity therefore cannot be compared from this capture.
The apparent grouping is a suspected divergence, not a confirmed one.

**Reading, not yet a fix.** The current core model treats `NEWLVL`/`CBIRTH` as
instantaneous: every same-type creature gets the identical `Q.TEN` delay
counted from the identical reference jiffy, so they all become ready on the
same tick. The observed dispatch sequence might be consistent with successive
births being queued at different times, but this capture cannot identify those
dispatches as one same-type batch or establish when their countdowns became
ready. Confirming that hypothesis needs slot/type identity and a jiffy-by-jiffy
trace of `CBIRTH`/`QUEADD` timing before any core change. Filed as #35,
spec-first per
`docs/prompts/track-r-rom-observation.md` step 5. No core change in this
commit.

**Labels.** `docs/specification/creatures.md` §3's claim that `CBIRTH` queues
`CMOVE` with the type's movement delay remains **source-proven**. This capture
does not validate the first dispatch's queue timing, nor whether a same-type
birth batch becomes ready simultaneously; both remain unresolved pending
relevant instrumentation.

**Backlog.** `docs/planning/capture-backlog.md` C-09 row updated to
`partial — track-r reconciliation; dispatch timing does not establish queue
delay or batch grouping; C-21 alignment and TCB/CBIRTH instrumentation needed`.

**Gate.** No ROM or capture staged: `t2-forward-corridor.rom.trace` predates
this session (C-01, 2026-09-25) and lives outside the tree under gitignored
`captures/` — this session captured nothing new, so nothing new needs
staging. `git status --porcelain` shows only the doc/backlog changes in this
commit.
