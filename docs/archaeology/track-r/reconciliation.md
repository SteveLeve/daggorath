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
55% fewer `CMOVE` entries — 3,420 ROM vs 7,591 core); this capture's own
trace shows the same texture —
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

## C-13 — faint and recovery timing

**Rule at stake:** `docs/planning/capture-backlog.md` C-13:
`docs/specification/combat-and-items.md` "Faint, recovery, death (`HUPDAT`)"
— "Not fainted and delay `<= 3` faints: the scheduler stops polling the
keyboard, and `PLAYER` eats any character already buffered. Fainted and
delay `> 4` recovers" — against `HUPDAT.ASM`'s exact heart-rate formula
(`J = floor(P*64/(P+D*2)), by repeated subtraction, minus 19`).

**Harness extension.** Two additions, both reused rather than adding new
game-behaviour surface:

- `dcli` gained a `--poke JIFFY:FIELD:VALUE` flag (`src/app/dcli.cpp`),
  mirroring `DOD_POKE` on the ROM side. It calls the existing
  `Game::set_player_damage`/`set_player_power` test hooks
  (`src/core/include/daggorath/game.hpp`) between two `advance_jiffies`
  calls — no new `Game` API, no behaviour change to either hook. This makes
  a "core" comparison trace possible for any future harness-modified poke
  capture, not just this one.
- `tools/rom/capture.lua` gained a `FAINT`/`REVIVE` trace emission, mirroring
  `src/core/game.cpp:418,423`'s wording exactly (`FAINT heart_rate=N` /
  `REVIVE heart_rate=N`), reading the already-watched `FAINT` byte
  (`tools/rom/watchlist.tsv`) with the same signed-byte interpretation
  `HUPDAT.ASM`'s `SUBA #19` needs (the core stores a signed reading; the tap
  now converts the raw byte the same way).

**Capture.** Level 0, Original Mode, no walk. `PDAM` is poked to 159 at
jiffy 10 (`PPOW` stays at its default 160, so this is just below the `BLO`
death threshold — `docs/specification/combat-and-items.md` "Death is `PPOW
< PDAM`" — with the least margin that still guarantees a faint: `HUPD00`'s
exact repeated-subtraction quotient at `P=160, D=159` is 22, giving `HEARTR
= 22-19 = 3`, `<= 3`). A `MOVE` command is typed at 3-jiffy spacing well
inside the expected fainted window (jiffies 30–42, per the C-12 pacing
finding) to test keyboard suspension. `scratch/c13-faint-recovery.script`,
not committed (reproducible from the jiffy numbers here).

```
dcli --script scratch/c13-faint-recovery.script --jiffies 400 \
    --poke 10:damage:159 --trace <core-trace>
DOD_JIFFIES=400 DOD_STEM=c13-faint-recovery DOD_POKE=10:PDAM:159:2 \
    tools/rom/run-capture.sh scratch/c13-faint-recovery.script
```

An earlier run with `PDAM` poked to 155 (more margin below the death
threshold) never crossed into the ROM's `FAINT` flag at all: `HSLOW` healed
it back above the boundary between the harness's samples before `HUPDAT`
next evaluated the flag. That result is itself informative (see below) but
is not the primary capture; 159 is.

**Observation.**

| | Core (`dcli`) | ROM (`coco2b`) |
|---|---|---|
| Poke | jiffy 10, `PDAM`=159 | jiffy 10, `PDAM`=159 |
| `HEARTR` first reads `<= 3` | jiffy 10 (same jiffy as the poke) | jiffy 11 |
| `FAINT` flag sets | jiffy 10 | **jiffy 56** |
| Typed `MOVE` (30–42) dispatches? | no (eaten) | no (eaten) |
| `REVIVE` | jiffy 65 | **jiffy 123** |

**Divergence: the ROM's `FAINT` flag lags its own `HEARTR` threshold by
tens of jiffies; the core's does not.** The ROM's `HEARTR` byte already
reads 3 (the exact `<= 3` faint threshold) at jiffy 11, one jiffy after the
poke — consistent with the usual one-jiffy alignment offset seen throughout
Track R. But the `FAINT` flag itself does not flip until jiffy 56, 45
jiffies later. Over that window `PDAM` keeps falling (`HSLOW` heals roughly
every 5 jiffies in this trace — `damage=156` at 11, `153` at 59, `150` at
63, ...), yet `HEARTR` stays pinned at 3 the whole time
(`captures/c13-faint-recovery.rom.trace`). The core's model runs
differently: `set_player_damage` calls `update_heart_rate()` synchronously,
so `FAINT` (and, implicitly, every faint/recover decision) is exact and
immediate on every `PDAM` change, with no equivalent gap. `REVIVE` shows the
same pattern at a larger scale: 55 fainted jiffies in the core, 67 in the
ROM.

**Reading, not yet a fix — reconciled against D-14's fade mechanism.** An
earlier draft of this entry attributed the 45-jiffy gap to `HUPDAT` running
on a periodic, heartbeat-paced schedule rather than on every `PDAM` change.
That is not the only explanation, and is likely not the right one:
`docs/specification/clock-and-scheduler.md` D-14 already establishes, from
C-17/C-18, that `HUPD30` performs the faint fade
as *foreground* work — one `RLIGHT`/`MLIGHT` step per pass, 5 jiffies per
step (C-17), continuing until `RLIGHT <= -8` — before the faint sequence
finishes. If `HUPDAT` actually runs immediately at jiffy 11 (matching the
core's synchronous model) and then spends the next several jiffies inside
that fade loop, the 45-jiffy gap to the `FAINT` flag's own write would be
mostly or entirely fade time, not idle time between `HUPDAT` invocations —
45 jiffies is close to 9 fade steps at 5 jiffies each, and 9 steps is a
plausible step count for `RLIGHT` descending from a small positive value to
`<= -8`. This capture did not tap `RLIGHT`/`MLIGHT` or `HUPD30`'s own entry
during the gap, so it cannot actually distinguish "periodic invocation"
from "immediate invocation plus fade delay before the flag write" — the
periodic-invocation framing above overstated what was shown. The honest
claim is narrower: a real, multi-jiffy gap exists between `HEARTR` crossing
the threshold and `FAINT` being written, and it is at least consistent with
the already-documented D-14 fade mechanism rather than necessarily a new,
separate `HUPDAT` scheduling behaviour. Confirming which needs an entry tap
at `HUPD30`/`HUPDAT`'s start, or a watch on `RLIGHT` across this specific
window. Filed as #51, spec-first per `docs/prompts/track-r-rom-observation.md`
step 5; #51 should be revisited against this reconciliation rather than
taken as confirming a periodic-schedule model. No core change in this
commit.

**Keyboard suspension: consistent, not conclusively isolated.** Both traces
show the typed `MOVE` command silently eaten — no `LINE`, no `MOVE` event,
in either trace, matching the specification's "`PLAYER` eats any character
already buffered." In the ROM, the keystrokes (jiffies 30–42) land after
`HEARTR` first read `<= 3` (jiffy 11) but well before the `FAINT` flag's own
latch (jiffy 56). That is consistent with keyboard suspension being gated
on `HEARTR`'s value directly rather than on the `FAINT` flag's own
transition, but this capture only shows the keystroke was eaten somewhere
in that 45-jiffy window, not at which specific jiffy the suspension took
effect — it cannot rule out the flag-gated reading either. Unresolved.

**Labels.** `docs/specification/combat-and-items.md`'s `HUPDAT` faint/
revive/keyboard-suspension text stays **[SRC]**; the underlying trigger
values (`delay <= 3` faints, `delay > 4` recovers) are now **ROM-observed**
in the sense that both transitions were seen to occur at exactly those
`HEARTR` readings, but the *timing* of when the `FAINT` flag is written
after a threshold crossing is not — a real multi-jiffy gap exists and is
unresolved, but this capture cannot show whether it is a `HUPDAT`
scheduling gap or D-14's already-documented fade delay (or both); not a
confirmation of either model.

**Backlog.** `docs/planning/capture-backlog.md` C-13 row updated to
`partial — track-r reconciliation: faint/recover threshold values
ROM-observed, but a real gap exists between the threshold crossing and the
FAINT flag write, not yet distinguished from D-14's fade delay, filed as
#51`.

**Gate.** `captures/c13-faint-recovery.rom.trace` and its companion
`.raw.tsv`/task logs are new but live outside the tree under gitignored
`captures/`. `src/app/dcli.cpp` gained the `--poke` flag and
`tools/rom/capture.lua` gained `FAINT`/`REVIVE` emission (both committed,
no ROM or capture bytes). `git status --porcelain` shows only the harness/
core/doc/backlog changes in this commit.

## C-15 — bare CLIMB

**Rule at stake:** `docs/planning/capture-backlog.md` C-15, and the
manual/source discrepancy `docs/archaeology/phase-0-archaeology-report.md`
flags: "Manual suggests `CLIMB` and `CLIMB UP`; source rejects empty
direction." `PCLIMB.ASM` checks `VFIND` first (`BMI PCLI00` — no feature
under the player rejects immediately, before `PARSER` ever runs) and only
then calls `PARSER` for the direction token, branching to the shared
`CMDERR` handler (`PARSER.ASM`) on a null/illegal token. `Game::cmd_climb`
(`src/core/game.cpp`) combines both checks with one `||`
(`feature < 0 || dir.status != ParseStatus::Matched`), which gives the same
final `???` either way but means a capture taken off a climbable feature
cannot tell which branch actually fired.

**Harness extension.** `tools/rom/capture.lua` gained one read-tap at
`CMDERR` (`CBE1` on this image, `SWI`/`FCB OUTSTI` = `3F 02`, verified
against `build/rom/daggorath.lst` before trusting it, same technique as
C-12's taps), emitting `OUTPUT ???` for every `???` reached through
`CMDERR` specifically — `CMDERR` is called via `JSR`/`JMP` from several
command handlers in the listing (`HUMAN.ASM`, `PCLIMB.ASM`, `PGET.ASM`,
`PTURN.ASM`), so this tap is reusable for any of those in a future capture,
not a claim that every `OUTSTI` call in the ROM is a `???` (`OUTSTI` is a
generic string-print reused for unrelated messages elsewhere, e.g.
`PEXAM.ASM`'s "IN THIS ROOM"). `dcli` separately gained a `position` field
on `--poke` (`row*256+col`), calling the existing `Game::place_player` test
hook, to place the player on a specific cell without a maze walk.

**First attempt — confounded.** The first run typed bare `CLIMB` at the
level-0 spawn cell (row 16, col 11) with no walk. `src/core/population.cpp`'s
`kVftTab`, decoded for level 0, has an empty up-list and a down-list of
`(0,23), (15,4), (20,17), (28,30)` — the spawn cell is on none of them, so
`vfind` (and, if the table matches, the ROM's `VFIND`) returns "no feature"
there. That run's `???` is consistent with `PCLIMB`'s `VFIND` short-circuit,
not with `PARSER`'s null-token rejection — it cannot tell the two apart, so
it does not actually answer the open question, which is specifically about
the missing-direction case. Caught before commit; not the capture below.

**Capture.** Level 0, Original Mode. The player is placed on one of the
level-0 down-list cells above (row 15, col 4) via `--poke`/`DOD_POKE`
before typing bare `CLIMB`, so a feature genuinely is present and `PARSER`
is the branch actually exercised. 3-jiffy keystroke spacing.
(`scratch/c15-bare-climb.script`, not committed — six lines, reproducible
from the jiffy numbers below.)

```
dcli --script scratch/c15-bare-climb.script --jiffies 100 \
    --poke 5:position:3844 --trace <core-trace>
DOD_JIFFIES=100 DOD_STEM=c15-bare-climb DOD_POKE=5:PROW:15:1,5:PCOL:4:1 \
    tools/rom/run-capture.sh scratch/c15-bare-climb.script
```

**Observation.** The poke lands in both: the ROM's per-jiffy watchlist
sample shows `MOVE row=15 col=4` at jiffy 4 (`captures/
c15-bare-climb.rom.trace`); `dcli`'s `place_player` test hook writes the
field directly with no trace line of its own, confirmed instead by the
core trace's `# final row=15 col=4` line. The bare `CLIMB` is still
rejected in both:

| | Core (`dcli`) | ROM (`coco2b`) |
|---|---|---|
| `LINE "CLIMB"` | jiffy 26 | jiffy 29 |
| `OUTPUT ???` | jiffy 26 | jiffy 30 |

The jiffy gaps between core and ROM here are larger than C-09/C-12/C-13's
usual single-digit alignment slack (a few jiffies each way); this capture
did not investigate why and treats it as the same unresolved C-21
alignment question, not a new finding.

**Labels.** The manual/source discrepancy is resolved in the source's
favor and is now **ROM-observed**: standing on a genuine climbable feature,
a bare `CLIMB` still produces `???` on real hardware — the `PARSER`
null-token rejection specifically, not just the no-feature short-circuit —
matching the source reading, not the manual's claim that bare `CLIMB`
climbs up. No divergence, no issue filed.

**Backlog.** `docs/planning/capture-backlog.md` C-15 row updated to
`captured — track-r reconciliation: bare CLIMB rejected on a genuine
climbable feature, matching source over manual`.

**Gate.** `captures/c15-bare-climb.rom.trace` and its companion
`.raw.tsv`/task logs are new but live outside the tree under gitignored
`captures/`. `tools/rom/capture.lua` gained one new tap and `src/app/
dcli.cpp` gained the `--poke position` field (both committed, no ROM or
capture bytes). `git status --porcelain` shows only the harness/core/doc/
backlog changes in this commit.

## C-14 — torch burn-out across a minute boundary

**Rule at stake:** `docs/planning/capture-backlog.md` C-14: `BURNER`
(`COMPLR.ASM`) — decrements the worn torch's timer once a minute, marking
it dead at `<= 5`. `docs/specification/combat-and-items.md`'s existing text
("At 5 or below the torch type becomes `DEAD`") was source-proven only.

**Harness extension.** Four additions:

- `dcli` gained a `--poke JIFFY:torch:VALUE` field, calling a new
  `Game::set_torch_timer` test hook (`src/core/include/daggorath/game.hpp`)
  that writes the worn torch's `spec[0]` (`P.OCXXX`) directly. No-op if
  nothing is worn.
- `dcli --poke` also gained `position` (`row*256+col`, via the existing
  `place_player` hook — folded in with C-14 rather than split out, since it
  was needed here too, to keep the player away from level 0's spawn-area
  creature traffic seen in C-12/C-13's captures).
- `tools/rom/capture.lua` gained an opt-in, ad hoc one-byte watch,
  `DOD_EXTRA_WATCH=NAME:HEXADDR`, emitting a `WATCH NAME=value` line on
  change. Unlike C-12/C-13/C-15's taps, the torch OCB has no fixed ROM
  symbol: `PTORCH` (added to `tools/rom/watchlist.tsv`) is a *pointer*, set
  only once the player wears the torch, to whichever `OCBLND` slot `OBIRTH`
  happened to allocate it at boot. A first, throwaway capture (jiffies=130,
  no torch poke) wore the torch and read `PTORCH`'s value from the raw
  sample (`0E95`) after it changed from `0000`; `P.OCXXX`'s offset (`+6`,
  `CD.ASM`) gives the timer field's address for *this specific, deterministic
  boot sequence* (`0E9B`), which `DOD_EXTRA_WATCH` then watches directly.
  This address is not a general-purpose symbol and is not assumed stable
  across a different script.
- `tools/rom/capture.lua` also gained `DOD_EXTRA_POKE=isr:hexaddr:value[:width]`,
  a *write*-side sibling to `DOD_EXTRA_WATCH`. `DOD_POKE`'s target must
  resolve through `sym()` against `build/rom/symbols.tsv` — a compile-time
  label, always. `0E9B` is not one (see above), so it cannot be a `DOD_POKE`
  target; a first draft of this capture worked around that by hand-appending
  a synthetic `TORCHTMR 0E9B` row to the *generated* `symbols.tsv`, which
  `tools/rom/assemble.sh` silently discards on its next run — a real
  reproducibility gap, caught in review. `DOD_EXTRA_POKE` writes a literal
  address directly and needs no symbol at all; the capture below was re-run
  with a freshly-`assemble.sh`'d `symbols.tsv` (confirmed to have no
  `TORCHTMR` row) to verify it reproduces the same result through the fixed
  mechanism.

**Capture.** Level 0, Original Mode: `PULL RIGHT PINE TORCH`, `USE RIGHT`
(wears it, setting `PTORCH`), then the timer is poked to 6 — one tick above
the dead threshold, so the very next `BURNER` run crosses it. `SECOND` is
also poked to 52 shortly after scheduler entry (a regular, non-`DOD_POKE_SECOND`
poke — that mechanism only affects `DGEN90`'s maze-generation seed, not the
running clock, discovered by trying it first and seeing `INIT` still report
`second=6`), so the level-0 spawn-area creature traffic seen in C-12/C-13
has only ~8 seconds to threaten the player before the minute rolls over,
rather than the ~54 seconds a fresh boot would need.
(`scratch/c14-torch-burnout.script`, not committed — 30 lines, reproducible
from the jiffy numbers below.)

```
dcli --script scratch/c14-torch-burnout.script --jiffies 550 --second 52 \
    --poke 115:torch:6 --trace <core-trace>
DOD_JIFFIES=550 DOD_STEM=c14-torch-burnout \
    DOD_POKE=2:SECOND:52:1 \
    DOD_EXTRA_POKE=115:0E9B:6:1 \
    DOD_EXTRA_WATCH=TORCHTMR:0E9B \
    tools/rom/run-capture.sh scratch/c14-torch-burnout.script
```

**Observation.** Both traces cross the dead threshold at exactly the
minute boundary:

| | Core (`dcli`) | ROM (`coco2b`) |
|---|---|---|
| Torch timer before `BURNER` (worn) | 15 (initial, matches the manual's nominal 15-minute pine-torch lifetime) | 15 (`WATCH TORCHTMR=15` at jiffy 0) |
| Timer poked to 6 | jiffy 115 | jiffy 114 |
| `BURNER` fires at the minute boundary, timer -> 5 | jiffy 480 (core also reports `TORCH dead timer=5`, a core-side inference) | jiffy 462 (`WATCH TORCHTMR=5`; type byte not watched) |

The ROM poke and its own watched read land on the jiffy *below* the isr
number passed to `DOD_EXTRA_POKE`/`DOD_POKE` (114, not 115) because
`sample()` labels each reading with `isr - 1`
(`tools/rom/capture.lua`, documented where `sample` is defined) — the
poke and the read it produces happen in the same harness call, at the same
isr, consistently with every other Track R capture's isr/jiffy convention;
this is not evidence of a timing gap. The 18-jiffy gap between core (480)
and ROM (462) at the minute-boundary dispatch itself is larger than
C-09/C-12/C-13's single-digit alignment slack; this capture did not
investigate why and treats it as the same open C-21 alignment question, not
a new finding.

**Unresolved: light-level propagation not observed in this window.** The
core's `task_burner` calls `refresh_light()` on every run, so its
`"light=5"` trace text reflects the *global* light level dropping in step
with the timer, the same jiffy. The ROM's watched `RLIGHT` (regular light
level) stayed at `07` for the entire capture, never dropping to 5.
`BURNER`'s own `BURN10`/`BURN20` only update the torch's *own* light-level
copies (`P.OCXXX+1`/`+2`); propagating that into the displayed `RLIGHT` is
`LUKNEW`'s job (`PUPDAT.ASM:17`, `STD RLIGHT`, called from `LUKNEW`'s
`LNEW10` path). `LUKNEW` is **not** proximity-scheduled: `COMPLR.ASM`'s
`LNEW99` unconditionally self-reschedules every `SCHED$ 3,Q.TEN` (three
tenths, twice a second, roughly every 18 jiffies) regardless of whether it
ran a `PUPDAT` that pass — only the `PUPDAT` *call itself* is gated on
`NEWLUK`/map-mode. The ROM's `TASK` log shows a `LUKNEW` dispatch every
~18 jiffies through jiffy 438, then none for the remaining ~112 jiffies of
the capture (438 to the 550-jiffy end, past the torch's death at 462) —
given the fixed cadence, that gap is itself odd and not explained by this
capture. It may be the same texture as #39's missing-dispatch finding
(C-11) rather than anything specific to `BURNER`/torches, but this capture
did not establish that. Unresolved either way: not claimed as a confirmed
divergence, and the apparent `LUKNEW` gap is flagged rather than explained
away.

**Labels.** The once-a-minute decrement is now **ROM-observed**: the torch's
timer, poked to 6, crosses to 5 at the very next minute boundary in both
traces, matching exactly. The `<= 5` *dead* threshold itself is **not**
promoted by this capture — `DOD_EXTRA_WATCH` only watched the timer byte
(`P.OCXXX`, `TORCHTMR`); it never watched the type byte (`P.OCTYP`) or
tapped the branch that stores `DEAD`, so the ROM side only shows
`WATCH TORCHTMR=5`, not confirmation that the torch's type actually became
`DEAD`. The core's `"TORCH dead"` line is a core-side inference from the
same threshold, not a ROM observation. The `<= 5` dead-type transition
stays **[SRC]** until the type byte or its branch is captured. The
light-level propagation question above is explicitly **not** resolved by
this capture. No issue filed for the alignment gap (folded into the
existing, open C-21 question); no issue filed for the light-propagation gap
(insufficient evidence either way, not a confirmed divergence).

**Backlog.** `docs/planning/capture-backlog.md` C-14 row updated to
`partial — track-r reconciliation: once-a-minute decrement ROM-observed;
<= 5 dead-type transition still [SRC] (only the timer byte was watched,
not the type byte or its branch); RLIGHT propagation not observed in this
capture's window`.

**Gate.** `captures/c14-torch-burnout.rom.trace` and its companion
`.raw.tsv`/task logs are new but live outside the tree under gitignored
`captures/`. `tools/rom/watchlist.tsv` gained `PTORCH`; `tools/rom/
capture.lua` gained the `DOD_EXTRA_WATCH`/`DOD_EXTRA_POKE` mechanisms; `src/core/include/
daggorath/game.hpp` gained `set_torch_timer`; `src/app/dcli.cpp` gained the
`torch` poke field (all committed, no ROM or capture bytes). `git status
--porcelain` shows only the harness/core/doc/backlog changes in this
commit.

## C-21 — start-up alignment

**Rule at stake:** `docs/planning/capture-backlog.md` C-21: why the ROM's
first `PLAYER` dispatch comes at isr 13–14 against jiffy 1 in the core, and
whether `GAME50`'s `INIVU`/`SYNC` (D-15, `docs/specification/
clock-and-scheduler.md`) explains it. This offset has complicated every
comparative capture so far (C-09, C-10, C-11, C-14): none could do a
literal line-by-line `trace_diff`, only anchor- or aggregate-based
comparisons.

**Source reading.** `GAME50` (`ONCE.ASM`) is `SWI FCB INIVU` then
`JMP SCHED` — no jiffy wait of its own. `INIVU` (`INIVUX`, `PLOOK.ASM`)
clears the status line and text area, runs `HUPDAT`, updates the status
line, sets `DSPMOD` to `VIEWER`, and falls into `PLOOK`'s own
`SWI FCB PUPDAT` before returning. `SCHED` itself (`COMMON.ASM`) is an
unbounded foreground loop with no interrupt wait between TCB dispatches —
confirming `docs/specification/clock-and-scheduler.md` §5's existing
source-proven reading. None of this, by itself, says how many real jiffies
`INIVU` (in particular, its full-screen 3D view draw) costs; that is a
question about 6809 cycle counts the listing alone doesn't answer, and this
capture does not attempt to derive it.

**Capture.** Level 0, Original Mode, no keystrokes at all (an empty script,
matching C-11's idle run but purpose-captured fresh and short for this
question specifically). `scratch/c21-startup.script` (empty), not
committed.

```
dcli --script scratch/c21-startup.script --jiffies 30 --trace <core-trace>
DOD_JIFFIES=30 DOD_STEM=c21-startup tools/rom/run-capture.sh \
    scratch/c21-startup.script
```

**Observation.** Both traces dispatch the same five system tasks together
as one "opening lap" — `PLAYER`, `LUKNEW`, `HSLOW`, `BURNER`, `CREGEN`, in
that order, all on the same jiffy, matching `Q.SCD`'s creation order in
`SYSTCB` (`ONCE.ASM`) — but at different jiffies:

| | Core (`dcli`) | ROM (`coco2b`) |
|---|---|---|
| Opening lap (`PLAYER`+`LUKNEW`+`HSLOW`+`BURNER`+`CREGEN`) | jiffy 1 | jiffy 11 |
| `PLAYER` re-dispatch cadence after that | every jiffy | every jiffy (12 through 24, unbroken) |

A **uniform 10-jiffy offset**, not just a `PLAYER`-specific one: every
system task in the opening lap is delayed by exactly the same amount. This
was also checked against the pre-existing `captures/idle-10min.rom.trace`
(C-11's 10-minute idle capture): its opening lap lands at jiffy 11 too,
same as this fresh capture — the offset is reproducible, not an artifact
of this specific short run.

**Reading, not a full resolution.** A uniform, task-independent delay
before the *entire* first scheduler lap is consistent with D-15's
hypothesis that `GAME50`'s `INIVU` call — specifically, the cost of
drawing the initial 3D view, not just its trailing `SYNC` — runs in the
foreground for several real jiffies before `JMP SCHED` is ever reached,
while `CLOCK`'s interrupt-driven jiffy counter keeps advancing regardless
(interrupts are not blocked by foreground drawing work). The harness's
"jiffy 0" anchor (first `CLOCK` after `GAME50` is *fetched*) therefore
measures the wrong start point for scheduler-relative comparisons: real
jiffies elapse while the ROM is still inside `INIVU`'s foreground code, not
yet at `SCHED`'s first task dispatch. This capture is consistent with that
mechanism and gives it its first ROM-observed jiffy count (10), but it does
not derive `INIVU`'s cost from 6809 cycle counts, so it cannot say whether
10 jiffies is `INIVU`'s exact cost or includes some other fixed overhead.

**Not resolved by this capture: the offset was not stable across scripts.**
The earlier C-01 ROM capture `t2-forward-corridor.rom.trace` (a script that
types `M` almost immediately) shows a *different* split: `PLAYER`'s own
first dispatch lands at jiffy 13 (not 11), and `LUKNEW`/`HSLOW`/`BURNER`/
`CREGEN` don't appear until jiffy 25, well after `PLAYER` rather than
alongside it. Two different capture scripts giving two different startup
patterns is exactly the kind of task-dispatch-schedule sensitivity to
script/input timing that #39 and #43 already flag as an open,
unmodeled area; this capture surfaces a third data point for that family
rather than resolving it. No new issue filed — folded into the existing
#39/#43 open question rather than duplicated.

**Labels.** No label promoted to ROM-observed: the *offset value* (10
jiffies, for an idle script) is a solid, reproducible observation, but the
open backlog question — "why" in mechanistic, cycle-exact terms, and
whether it is stable enough to use as a correction factor for `trace_diff`
— remains unresolved. `docs/specification/clock-and-scheduler.md`'s D-15
entry already carries the right label (**[INF]**) and is not changed by
this capture; this reconciliation entry is additional evidence for that
existing inferred note, not a promotion of it.

**Backlog.** `docs/planning/capture-backlog.md` C-21 row updated to
`partial — track-r reconciliation: a uniform 10-jiffy opening-lap offset is
ROM-observed for an idle script, consistent with D-15's INIVU-cost
hypothesis, but the offset was not stable across scripts (C-01's
`t2-forward-corridor` splits differently) and no correction factor for
`trace_diff` is established`.

**Gate.** `captures/c21-startup.rom.trace` and its companion `.raw.tsv`/
task logs are new but live outside the tree under gitignored `captures/`.
No harness or core changes in this commit — existing taps and watches were
sufficient. `git status --porcelain` shows only the doc/backlog changes in
this commit.

## C-19 — a creature on the player's cell

**Rule at stake:** `docs/planning/capture-backlog.md` C-19: `CMOVE`'s
`PUPDAT`/`SYNC` foreground cost and next-task timing when a creature ends
up on the player's cell. `docs/specification/clock-and-scheduler.md`'s D-15
entry already carries this as **[INF]**, citing `CMOVE`'s attack branch
(`CRETUR.ASM` `CMOV20`) and its "just arrived" branch (`CMOV90`) as the two
places `PUPDAT` fires for this case, and names "ROM capture C-19 is
pending."

**Harness extension.** Three additions, following the C-14/C-15 pattern:

- `Game::place_creature(slot, row, col)` (`src/core/include/daggorath/
  game.hpp`) and a `dcli --poke JIFFY:creature6:VALUE` field
  (`row*256+col`), writing `ccbs_[6]`'s position directly — slot 6
  specifically, matching the ROM address computed below, not a general
  facility.
- `tools/rom/capture.lua`'s `SCHED_JSR` tap gained creature slot identity.
  Previously every creature task logged as bare `CMOVE`, with "no slot or
  type identity" (an C-09 finding). `P.TCDTA` (TCB+5, `CD.ASM`) points at
  the dispatched creature's CCB; `CCBLND`/`CC.LEN` (both resolved compile-time
  symbols) turn that pointer into a slot index, `(ccb - CCBLND) / CC.LEN`,
  producing the same `CMOVE-N` naming the core's trace already uses. This
  is a general improvement, not C-19-specific, and changes the trace
  format for every capture that dispatches creatures from here on.
- A second new tap, at `CMOV20+11` (`D078` on this image, `SWI`/`FCB
  SOUNDS` = `3F 1C`, verified against `build/rom/daggorath.lst`) — the
  creature-attack sound, `CRETUR.ASM`'s counterpart to C-12's player-swing
  tap, at a different address because it's a different routine. Without
  this tap, a ROM capture cannot tell "the attack check ran and missed"
  apart from "the attack check never ran" — both leave `PDAM` unchanged.

CCBLND slot 6's fields were confirmed against a real boot with
`DOD_EXTRA_WATCH` before use: type 1 (viper), starting position (row 6, col
4) — a live, in-use creature, not an empty slot.

**Capture.** Level 0, Original Mode, no keystrokes. Slot 6's creature is
poked onto the player's spawn cell (row 16, col 11) shortly after
scheduler entry, before its own first `CMOVE` dispatch (which C-09/C-11
established happens no earlier than the first birth-batch wave, well after
the poke).

```
dcli --script scratch/c19-step-onto.script --jiffies 150 \
    --poke 5:creature6:4107 --trace <core-trace>
DOD_JIFFIES=150 DOD_STEM=c19-step-onto DOD_EXTRA_POKE=5:0449:16:1,5:044A:11:1 \
    tools/rom/run-capture.sh scratch/c19-step-onto.script
```

**Observation.** Both traces detect the co-location and enter the attack
path; the mechanism matches, the outcome and cadence differ:

| | Core (`dcli`) | ROM (`coco2b`) |
|---|---|---|
| First `CMOVE-6` dispatch | jiffy 85 | jiffy 78 |
| Attack sound (unconditional, before the hit/miss roll) | `SOUND slot=6 type=1` | `SOUND creature_class=1` |
| First attack outcome | miss (`MISS slot=6 roll=49`) | no `PDAM` change (miss, by elimination — see below) |
| Second `CMOVE-6` dispatch within the 150-jiffy window | jiffy 127 | none |
| Second attack outcome | hit, `damage=35` | n/a |

The ROM shows no `EXERT` (no `PDAM` change) after the jiffy-78 attack, and
the new `CMOV20+11` tap confirms the attack sound *did* play — so the
absence of damage is a miss, not the attack check failing to run. This
is the same reasoning C-12 already established for the player's own swing
(exertion/sound fires regardless of hit; damage only follows a hit), applied
here to the creature side for the first time.

**Divergence: no second attack attempt in the ROM's window.** The core's
viper (`CDBTAB` `attack_delay_tenths` 7, i.e. 42 jiffies) re-attacks at
jiffy 127 — 85 + 42, exactly on schedule — and hits. The ROM shows nothing
for `CMOVE-6` between jiffy 78 and the end of the 150-jiffy window: no
second dispatch, no second attack sound. This reads as the same texture as
#39's already-open finding (creatures dispatching less often than the core
predicts), not a new mechanism; no new issue filed.

**Not covered by this capture: `CMOV90`'s "just arrived" branch.** D-15
names two `PUPDAT` sites for a creature on the player's cell: `CMOV20`
(attack, captured above) and `CMOV90` (the creature's own movement code,
reached only when a `CWALK` step *lands* the creature on the player's
cell, not when it was already there — see `CRETUR.ASM` line 195 onward).
This capture poked the creature directly onto the cell, which exercises
`CMOV20` on its very next dispatch but never exercises `CMOV90`'s branch,
which needs the creature to actually take a winning step while adjacent —
a maze-pathing condition this capture did not attempt to arrange.
Unresolved, not claimed either way.

**Labels.** `CMOVE`'s attack-path mechanism (unconditional attack sound
before the hit/miss roll, independent of outcome) is now **ROM-observed**,
matching the core's parallel construction on the player's own `PATTK`
side. This capture's tap was at the attack sound (`CMOV20+11`), not at
`PUPDAT` itself, so `PUPDAT`'s role on the attack path stays **[INF]**.
The dispatch-cadence divergence is **not** promoted or filed separately —
folded into #39. `CMOV90`'s specific "just arrived" `PUPDAT`/`NEWLUK`-clear
branch remains **[INF]**, not exercised by this capture.

**Backlog.** `docs/planning/capture-backlog.md` C-19 row updated to
`partial — track-r reconciliation: CMOVE's attack path (unconditional
attack sound before the hit/miss roll) ROM-observed via a new
creature-attack sound tap; PUPDAT itself not tapped, CMOV90's "just
arrived" branch not exercised; dispatch-cadence gap folded into #39`.

**Gate.** `captures/c19-step-onto.rom.trace` and its companion
`.raw.tsv`/task logs are new but live outside the tree under gitignored
`captures/`. `tools/rom/capture.lua` gained CMOVE slot identity and the
`CMOV20+11` tap; `src/core/include/daggorath/game.hpp` gained
`place_creature`; `src/app/dcli.cpp` gained the `creature6` poke field (all
committed, no ROM or capture bytes). `git status --porcelain` shows only
the harness/core/doc/backlog changes in this commit.
