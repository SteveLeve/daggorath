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
input — is the same premise C-09 flagged as a suspected (not confirmed)
divergence, and these two captures add a further, script-level data point
against it (the tail sequence shifted between run 1 and run 2, so it isn't
simply "the same fixed schedule, offset from a different anchor"). Second,
and more specifically:
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

## C-12 — hit/miss sequence against a spider with the wooden sword

**Rule at stake:** `docs/planning/capture-backlog.md` C-12: `ATTACK`,
`DAMAGE`, `SCAL16` (`docs/specification/combat-and-items.md`), against the
level-0 starting wooden sword and a type-0 (spider) creature.

**Harness extension.** No existing tap distinguishes a connecting swing from
a miss: the only watched byte that changes on a hit is the *defender's*
damage (`CCBLND`, an unimplemented structure walk), not the player's `PDAM`.
`PATTK.ASM`'s per-event `SWI` dispatches give a byte-exact substitute, the
same technique already used for `PSTEP`'s `A$THUD` tap
(`tools/rom/capture.lua`). Addresses were taken from `build/rom/symbols.tsv`
and `build/rom/daggorath.lst` (LWTOOLS 4.25, this session's `assemble.sh`
run), not guessed, and each tap asserts its expected opcode bytes before
trusting the address:

| Tap | Address (this image) | Bytes | Meaning |
|---|---|---|---|
| `SWING_SWI` | `PATT10+30` (`D2E0`) | `3F 1C` | `SWI`/`FCB SOUNDS`: the swing's own object sound, class byte still in `A` |
| `HIT_SWI` | `PATT24` (`D31F`) | `3F 1B 12` | `SWI`/`FCB ISOUND`/`FCB A$KLK2`: the connecting-hit sound |
| `KILL_SWI` | `PATT40+10` (`D344`) | `3F 1B 15` | `SWI`/`FCB ISOUND`/`FCB A$EXP0`: the kill sound, after `PUPDAT` |

Each tap emits the same `SOUND` trace line the core (`src/core/game.cpp`)
already emits at the matching point, so the two traces' vocabulary lines up
directly: `SOUND class=N`, `SOUND A$KLK2`, `SOUND A$EXP0`. One unit
mismatch to note for future reads: the core's `class=` value is the object's
raw `P.OCCLS` (`game.cpp:1238`), but the tap reads the `A` register at the
`SWI`, which already has `SNDOBJ` (12) added — the wooden sword's `P.OCCLS`
is 4 (`src/core/game.cpp` `kClassWeight[4]`, the sword's weight), so `class=4`
in the core trace and `class=16` in the ROM trace are the same event, not a
divergence.

**Keystroke-pacing finding, methodology.** The first capture attempt reused
`docs/archaeology/phase-3/traces/fight-to-kill.script`'s walk (one keystroke
per jiffy) with a `PULL RIGHT WOODEN SWORD` / `ATTACK RIGHT` tail at the same
pace. Against the real ROM this corrupted multi-letter words with doubled
adjacent letters or long runs: `ATTACK` arrived as `ATACK` (all three
attempts), `WOODEN` as `WODEN`, and an entire `TURN LEFT` / `MOVE` pair was
lost, merging into a garbled `LINE "AZMOVE"`. `fight-to-kill.script` itself
was generated by re-simulating against the core (Phase 3 reconciliation) and,
per the backlog, was never previously run against the ROM — this is the
first time that pacing was tested against real hardware, and it does not
survive. Widening the gap to 3 jiffies between keystrokes (same commands,
same walk) reproduced every command cleanly in the ROM trace. This is a
capture-harness finding, not a game-behaviour claim, and is recorded here so
future Track R scripts use the wider spacing rather than copying
`fight-to-kill.script`'s pace.

**Capture.** Level 0, Original Mode: walk to the nearest type-0 (spider)
creature (`MOVE` x4, `TURN LEFT`, `MOVE` x2 — the same route as
`fight-to-kill.script`), `PULL RIGHT WOODEN SWORD`, then `ATTACK RIGHT` up to
three times, 3-jiffy keystroke spacing (`scratch/c12-spider-wooden-sword.script`,
not committed — reproducible from the jiffy numbers below). Compared against
a freshly generated core trace of the same script (`dcli --script ... --jiffies
400`).

```
DOD_JIFFIES=400 DOD_STEM=c12-spider-wooden-sword tools/rom/run-capture.sh \
    scratch/c12-spider-wooden-sword.script
```

Output: `captures/c12-spider-wooden-sword.rom.trace` (gitignored, not
staged).

**Observation — mechanically clean, outcome diverges.** Both traces execute
the walk, `PULL`, and all three `ATTACK RIGHT` commands; the tap addresses
above fired exactly once per swing with no unexpected extra or missing
`SOUND` line, and `PULL RIGHT WOODEN SWORD` equips object 63 (`LINE`/`PULL`
in the core trace) confirming the wooden-sword path, not the empty hand,
executes `ATTACK`/`DAMAGE`/`SCAL16` on every swing. The hit/miss outcome
diverges completely:

| | Core (`dcli`) | ROM (`coco2b`) |
|---|---|---|
| Swing 1 | miss | miss |
| Swing 2 | **hit, kill** (`A$KLK2` + `A$EXP0`, same jiffy) | miss |
| Swing 3 | (creature already dead; exertion only) | miss |

The ROM shows zero connecting hits across all three swings
(`grep -c 'A$KLK2\|A$EXP0' captures/c12-spider-wooden-sword.rom.trace` is 0);
the core predicts a kill on the second swing.

**Reading, not yet a fix.** `ATTACK`'s hit/miss test draws one `RANDOM` byte
per swing (`docs/specification/combat-and-items.md`), so the outcome depends
on the RNG stream's exact position when the swing runs — which in turn
depends on every RNG draw made by every task dispatched since level entry.
C-11 (issue #39) already found the ROM's task-dispatch schedule diverges
substantially from the core's over an idle run (missing `PLAYER` entries,
21% fewer `CMOVE` entries); this capture's own trace shows the same texture —
dense runs of `TASK run PLAYER` interleaved with repeated `TASK run CMOVE` in
the same jiffy (e.g. three separate `CMOVE` entries at jiffy 90, 96, 102)
in the approach to the spider, before either swing. A dispatch-schedule
difference this large, this early, is sufficient on its own to desynchronize
the RNG stream well before the first `ATTACK` draw, without needing any
error in the `ATTACK`/`DAMAGE`/`SCAL16` transliteration itself. This capture
cannot distinguish that explanation from a genuine error in the hit-test
math; it did not watch `SEED` at the swing jiffies specifically (the
watchlist samples `SEED` every jiffy, but confirming or ruling out RNG
desync needs the `SEED` value compared at the exact draw, not eyeballed from
the per-jiffy sample). Filed as #49, tied to #39, spec-first per
`docs/prompts/track-r-rom-observation.md` step 5. No core change in this
commit.

**Labels.** That a wooden-sword swing runs the same `PATTK` routine path as
the empty hand's, just with the sword's class/offense values, stays
**[SRC]** — that is the listing's unconditional `PATT10` fallthrough, not
something this capture's single wooden-sword run could establish on its own
(no empty-hand ROM run was captured here to compare against). Within that
shared path, the `ATTACK`/`DAMAGE`/`SCAL16` *dispatch structure* — the SWI
sites and their byte encoding — is now **ROM-observed** for the wooden-sword
case specifically (the tap assertions above would `die()` if the real ROM's
bytes did not match the assembled listing at these addresses, and they
matched on the first run). The *hit/miss outcome* is **not** promoted: this
capture does not confirm or refute `ATTACK`'s percentage-index math against
the ROM, only that the observed outcome differs, plausibly for a reason
external to that routine.

**Backlog.** `docs/planning/capture-backlog.md` C-12 row updated to
`partial — track-r reconciliation: SWI dispatch structure ROM-observed, but
the hit/miss outcome diverges and is not confirmed as ATTACK/DAMAGE/SCAL16
error vs. RNG desync (linked to #39)`.

**Gate.** `captures/c12-spider-wooden-sword.rom.trace` and its companion
`.raw.tsv`/task/sound logs are new but live outside the tree under
gitignored `captures/`. `tools/rom/capture.lua` gained three new taps
(committed, no ROM or capture bytes). `git status --porcelain` shows only
the harness/doc/backlog changes in this commit.
