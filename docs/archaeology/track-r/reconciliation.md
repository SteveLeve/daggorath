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

**Observation — task dispatch counts over the run:**

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

**Divergence: `PLAYER` does not dispatch every idle jiffy in the ROM.** The
core dispatches `PLAYER` in all 36000 jiffies of this idle script — with no
keystrokes, `PLAYER` re-queues on `Q.JIF` and runs again every single jiffy.
The ROM capture dispatches `PLAYER` in only 28444 of the 35990 jiffies between
its first and last logged `TASK` line (jiffy 11 to 36000) — about 79%.

Looking at *any* task dispatch, not just `PLAYER`: the ROM trace has a `TASK`
line in 29762 of those 35990 jiffies. **6228 jiffies (17%) show no task
dispatch of any kind.** The remaining gap between "any task" (29762) and
`PLAYER` alone (28444) — about 1318 jiffies — are jiffies where some other
task (`CMOVE`, `HSLOW`, `LUKNEW`, `CREGEN`, `BURNER`) ran but `PLAYER` did
not; 3948 jiffies had more than one task dispatch, so these aren't
mutually exclusive categories.

**Reading, not yet a fix.** The core models `PLAYER` as always re-queuing and
running on `Q.JIF` regardless of what else is ready that jiffy (consistent
with `PLAYER` counting 36000/36000). The ROM data is consistent with a
stricter reading of ADR-0002's "one bounded lap per jiffy": if `SCHED`
dispatches at most one ready task per interrupt in priority order, and
`PLAYER`'s `Q.JIF` slot can lose that single dispatch to a higher- or
differently-ordered task the same jiffy, `PLAYER` would skip jiffies exactly
when something else is ready — which is roughly, but not exactly, what the
1318-jiffy overlap shows. It does **not** explain the 6228 completely silent
jiffies: if `PLAYER` is unconditionally re-added to `Q.JIF` every jiffy (as
the core assumes and as `docs/specification/clock-and-scheduler.md` states
source-proven), some task should be ready and dispatched on every jiffy,
never zero. Either `PLAYER`'s re-queue is not unconditional (a rule this
project has treated as source-proven), or the capture's `SCHED_JSR` read tap
(`tools/rom/capture.lua`, fires on opcode fetch of the JSR into a task's
`P.TCRTN`) misses some dispatches — for instance if `SCHED` has more than one
code path into a task and only one is tapped. Both are open; neither is
confirmed here.

This is a reading of the raw counts, not a confirmed mechanism, and it is
larger and less clean than C-09's finding. It needs its own capture (a watch
on `Q.JIF`'s head pointer each jiffy, not just the dispatch PC) before any
spec or core change. Filed as #39, spec-first per
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
