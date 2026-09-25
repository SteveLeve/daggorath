# Archaeology reconciliation — Phase 1

Phase 1's questions were the ROM equivalence of the pinned listing, the
`CREGEN` birth blocker, and the population behaviour `NEWLVL` actually
performs. Items not investigated stay open.

**ROM.** LWTOOLS 4.25 assembled the pinned listing to the same 8192 bytes as
the Tandy catalog 26-3093 image. The diff is in
[`../../provenance/rom-diff.md`](../../provenance/rom-diff.md). On 2026-09-25
MAME 0.264 `coco2b` ran that image. §1 records the captures. A claim below is
ROM-observed only where §1 says so. Everything else is still resolved against
the listing at `a94326f`.

Confidence: **High** = read off the listing and cross-checked by the Python
extractor and the C++ core; **Medium** = read off the listing, one
implementation; **Low** = inference. ROM rows cite the capture, not a second
implementation.

## 1. ROM captures

The harness is `tools/rom/assemble.sh`, `tools/rom/capture.lua`,
`tools/rom/watchlist.tsv`, `tools/rom/trace_diff.py` and
`tools/rom/run-capture.sh`. Raw samples and traces are in gitignored
`captures/`. The five Phase 0b scripts were recaptured with the scheduler,
sound, and population logs. Two earlier runs of those scripts, before the
task log, were byte-identical; the traces below are the later run.

**Alignment (inferred).** The cartridge boots into the autoplay demo.
The harness holds SPACE until the demo's `CLOCK` transfers to `GAME`.
Jiffy 0 is the first `CLOCK` after `GAME50` is fetched. Keys are injected
at the PIA. `TASK` and `LINE` are emitted when `SCHED_JSR` (`C208`) and
`HMAN50` are fetched. Other state lines are still the first sample that
sees the RAM change, which is the following interrupt. The literal
`trace_diff.py` result is reported first. `--relative-to-init` is a second
comparison and is not the gate.

The rows below are the comparison against the reference that still started at
`0:0:1.0.0`. The owner then decided to count the 377 interrupts, and the
reference `INIT` line is now the ROM sample `0	0:0:6.2.5	INIT	level=0 row=16 col=11 dir=N second=6`.
The old baseline's first divergence, before that regeneration, is the line
this table names. The captures were not re-run. Divergences already measured
after skipping `INIT` stay the `--relative-to-init` paragraph under the table.

| Capture | First divergence (`trace_diff.py`, pre-decision reference) |
|---|---|
| `t1-move-turn-look` | line 1, jiffy 0: `INIT` clock `0:0:1.0.0` (reference) vs `0:0:6.2.5` (ROM) |
| `t2-forward-corridor` | same line and values as t1 |
| `t3-burst-one-jiffy` | same line and values as t1 |
| `t4-parser-edges` | same line and values as t1 |
| `t5-keyboard-overrun` | same line and values as t1 |
| Level entry at `SECOND` = 0, 1, 7, 30, 59 | harness-modified, level 1. See below. Maze bytes match the fixture at every value. Post-`DGEN90` seeds differ. `SECOND` = 0 took 256 draws |
| Level re-entry | none for the maze bytes (level 0 matches `maze-level-0.bin` on entry and on return). Creature placement differs. See below |
| Five-minute `CREGEN` boundary | the opening increment is the only matrix change through isr 19520 (clock about `0:5:22`). The rescheduled lap did not run. Leave-and-return is the ladder capture, not this idle |
| Simultaneous expirations | none as a trace diff (no reference script). Executed order is below |
| MOVE/TURN/LOOK timing (D-4) | measured. The literal trace diff is still the `INIT` line |
| Blocked MOVE exertion | consistent at one weight (`POBJWT` = 35). Second weight not run: `GET` adds weight only for an object on the current cell, and the opening sword and torch are in the bag |
| `SNOISE` and sound duration | `THUD` measured. `SNOISE` does not change `SEED`. A creature sound was not separately captured |

`--relative-to-init --kinds LINE,TURN,MOVE,EXERT` still diverges, and that
divergence is reported after the literal one. For `t1` it is reference
`LINE "T R"` at jiffy 9 versus ROM jiffy 15. For `t2` it is `LINE "M"` at
jiffy 7 versus 13. The ROM does not reach `SCHED` until about isr 11, so
early script keys wait. That is later than `INIT` and does not replace it.

**Level-0 build and issue #13 (ROM-observed).** `DGEN90` during the build,
before `GAME50`:

| | Value |
|---|---|
| Interrupts, `GAME10` `IRQSYN` to `GAME50` | 377 |
| Clock at the `INIT` sample | `0:0:6.2.5` |
| `SECOND` at `DGEN90` entry | 6 |
| Draws (`DGEN90` fetches) | 6 |
| `SEED` at entry | `3ACBDC` |
| `SEED` at the `RTS` after the loop | `8FC8AD` |
| `SEED` at `NEWLVL` exit / `GAME50` | `0766CB` |
| Live creatures at `GAME50` | 24 |
| `CMXLND` level-0 row at `GAME50` | `09 09 04 02 00 00 00 00 00 00 00 00` |

The C++ core's Original Mode entry applies those 377 interrupts before
`DGEN90`, then `birth_creatures`. It produces the three seeds and the same 24
positions, starting `0:3@28,5`. On the ROM the opening `CREGEN`, at isr 11
with `SEED` `0766CB`, increments type 5. The row becomes
`09 09 04 02 00 01 00 00 00 00 00 00`, the live count stays 24, and `SEED`
becomes `C30766`. The core's opening lap does the same. The next level-0
`NEWLVL` births 25.

**Applied (owner, 2026-09-25).** Count the 377 build interrupts. This is the
reason for the reference-trace regeneration that follows: the old baseline's
first divergence is jiffy 0, `INIT` clock `0:0:1.0.0` versus the ROM's
`0:0:6.2.5`. `population-entry.txt` stays the source-derived comparison,
including `SECOND` = 1 and `cregen 24 24 25 9 25`. It is not the ROM level-0
entry. The ROM-observed entry is the core's Original Mode constructor
(`SECOND` = 6), checked against the table above. No fixture hash was changed
to absorb this decision. The stored traces are regenerated only after this
paragraph.

**Dispatch to state change (ROM-observed).** Times are the `LINE` jiffy at
`HMAN50` and the jiffy of the later RAM sample.

| Script | Command | Dispatch | State |
|---|---|---|---|
| t1 | `T R` | 15 | facing E at 16 |
| t1 | `T A` | 94 | facing W at 95 |
| t2 | `M` | 13 | (15,11) at 20 |
| t2 | `M` | 41 | (14,11) at 47 |
| t2 | `M B` | 78 | (15,11) at 84 |
| t1 | blocked `M` | 31 | `PDAM` 7 at 52 |
| t1 | blocked `M` | 61 | `PDAM` 14 at 82 |

`POBJWT` stayed `0x0023` (35). `(35 >> 3) + 3` = 7, then 14. One weight only.

**Sound (ROM-observed).** The least play that triggers a sound is the blocked
step already in `t1`. `THUD` at `PSTEP`'s `SWI` / `ISOUND`:

| | First | Second |
|---|---|---|
| Start isr | 35 | 65 |
| End isr | 50 | 79 |
| Interrupts the foreground was inside the sound | 15 | 14 |
| `SNOISE` calls | 208 | 208 |
| DAC writes at `$FF20` | 105 | 105 |
| `SEED` before and after | `C30766` | `C30766` |

Across 407 `SNOISE` returns in the `t1` capture, `SEED` was unchanged on
every call. `SNDRND` is a different two-byte state and does change.

**Simultaneous expirations (ROM-observed).** Idle capture
`cregen-leave-return`. `BURNER` is the minute task (delay 1). `LUKNEW` is
the tenth task. `PLAYER` is the jiffy task. Same-interrupt order:

| isr | Tasks, in dispatch order |
|---|---|
| 3222 | `PLAYER`, `LUKNEW`, `BURNER` |
| 6822, 10422, 14022, 17622 | `PLAYER`, `BURNER` |

Jiffy, then tenth, then minute, on the interrupt that scanned those queues.
No hour task ran. No second-queue task expired on these boundaries, so FIFO
inside one queue was not a multi-task sample. `CREGEN` did not run on any
of these interrupts.

**Level entry, re-entry, `CREGEN` (ROM-observed, with one labeled poke).**
The route is `docs/archaeology/phase-1/scripts/descend-early.script`: from
(16,11) facing north to the ladder at (0,23), derived from
`maze-level-0.bin` and `vertical-features.json`. The nearer holes are
down-only (`PCLIMB` accepts an upward ladder). `CLIMB DOWN` then `CLIMB UP`.

| Point | Level | `SECOND` | Live | Maze |
|---|---|---|---|---|
| `NEWLVL` exit, build | 0 | 6 | 24 | matches `maze-level-0.bin` |
| After opening `CREGEN` | 0 | 6 | 24 | unchanged; type 5 count is 1 |
| `NEWLVL` exit, descent | 1 | 41 | 24 | matches `maze-level-1.bin` |
| `NEWLVL` exit, return | 0 | 50 | 25 | matches `maze-level-0.bin` again |

Return positions are not the entry positions (first live block `0:3@28,5`
on entry, `0:5@18,7` on return). The extra creature is born by that
`NEWLVL`, from the matrix the opening lap already incremented. `SYSTCB`
runs again on the return, so `CREGEN` runs again immediately and increments
the level-0 row a second time (`00 01` becomes `00 01 01`) without birthing.

`SECOND` = 0, 1, 7, 30, and 59 were not reachable on unmodified level 0.
The harness wrote `SECOND` at the `LDB SECOND` immediately before `DGEN90`,
and only on spin 2 (the descent). Spin 1, the level-0 build, was not
written. These captures are **harness-modified state**. The level-1 maze
bytes still match `maze-level-1.bin` at every value. Entry `SEED` of that
spin was `DFED3B` each time. Draws and the post-loop seed:

| Written `SECOND` | Draws | `SEED` after the loop | First creature at `NEWLVL` exit |
|---|---|---|---|
| 0 | 256 | `2A2693` | `0:5@28,6` |
| 1 | 1 | `55DFED` | `0:5@27,12` |
| 7 | 7 | `FB0C52` | `0:5@4,30` |
| 30 | 30 | `F7BB47` | `0:5@31,0` |
| 59 | 59 | `8C2C8C` | `0:5@1,18` |

The `SECOND` = 0 spin is the 256-draw case. The clock's `SECOND` byte was
the cell that was written, so the clock after that instruction is not an
unmodified clock.

**Five-minute window.** From the opening `CREGEN` at isr 11 through isr
19520, level-0 `CMXLND` stayed `09 09 04 02 00 01 …`. `BURNER` ran on five
minute boundaries (isr 3222, 6822, 10422, 14022, 17622). `CREGEN` did not.
The source schedules it `SCHED$ 5,Q.MIN` after the opening lap. Why that
second lap did not fall on one of those five scans is **unresolved**. The
idle did get past five minutes: the walk at jiffy 18000 sees clock
`0:5:6.3.2`. The walk itself did not finish. At isr 19522 the clock reads
`0:0:0.0.0` and a new level-0 `NEWLVL` follows (fresh row, 24 creatures).
That is a restart, recorded and not interpreted further. The
leave-and-return result is the ladder capture above, which is the opening
increment, not this five-minute reschedule.

**t3 and t5.** Reference-only. `POLCAT` delivers at most one key per
interrupt, so a one-jiffy burst is not a keyboard replay. No buffer-level
injection was added.

D-1 and D-2 stay open for the cases named in §13. Their queue order is no
longer unmeasured. D-4's durations are measured; the core still spends no
time, so the deviation stays. D-5 and D-6 are unchanged.

The committed Phase 0b traces were not regenerated. `CREGEN` does not emit
a trace line and `CMOVE` is not queued, so those traces still match the core.

## 2. How a matrix increment becomes a creature

| Question | Result | Confidence |
|---|---|---|
| Does `CREGEN` create a control block? | **No.** It increments `CMXLND[LEVEL][(RANDOM & 7) + 2]` when the 8-bit sum of that row is below 32, then reschedules at 5 minutes. | High |
| When does the extra creature exist? | The next `NEWLVL` for that level. `NEWLVL` zeros the control blocks and calls `CBIRTH` once per current matrix count. The matrix is not recopied from `CMTTAB`. | High |
| Does this include returning to a level? | Yes. `CMXLND` survives the level change. Time on another level does not increment this row, because `CREGEN` uses `CMXPTR`. | High |
| Why does one increment happen before five minutes? | `SYSTCB` puts every system task in `Q.SCD` with countdown 0, so the opening lap runs `CREGEN` once. On level 0 at `SECOND = 1` that draw increments type 9. Live count stays 24 until re-entry, which births 25. This row is the source-derived `SECOND = 1` case. The ROM opening lap, at `SECOND` = 6, increments type 5 and is §1. | High for the source case. The ROM lap is §1 |

`fixtures/population-entry.txt` records the level-0 case as
`cregen 24 24 25 9 25`. The C++ test checks those five numbers.

## 3. Population established from source

| Question | Result | Confidence |
|---|---|---|
| Initial counts | `CMTTAB` rows sum to 24, 24, 23, 24, 31. Birth order is type 11 down to 0. | High |
| Where is a creature placed? | `RNDCEL` (column, then row) on a cell that is not `$FF` and not already occupied. The block is marked in use while its row and column are still 0, so (0, 0) is occupied by itself and is never used. | High |
| Do positions depend on `SECOND`? | Yes. The maze does not. Confirmed for level 0 at 0, 1, 7, 30, 59. | High |
| Object distribution | `ONCE` walks `OMXTAB` before the first `NEWLVL`. High nibble is the start level, low nibble the count, and the level wraps back when it passes 5. Ownership `$FF` means creature-owned. Shields, swords, and torches are overwritten with the leather, wooden, and pine definitions; the original reveal requirement is restored. | High |
| Attachment | On the current level only, each creature-owned object is prepended to the next live creature, wrapping through the 32 slots. The player's sword and torch are created after the first `NEWLVL` and are not in that attachment. | High |
| Vertical-feature pointer | `NEWLVL` leaves `VFTPTR` at byte indexes 0, 1, 14, 27, 28 for levels 0–4. The empty 2↔3 link is two consecutive `$80` bytes. | High |

`vertical-features.json` gained `newlvl_vftptr_byte_index`. Its previous bytes
are unchanged. `population.json` and `population-entry.txt` are new. Other
fixture hashes are unchanged. `MANIFEST.json` was regenerated for those
additions. No baseline was regenerated to hide a mismatch.

## 4. Quirks recorded this phase

The canonical copy of this table, together with the Phase 0b rows, is
[`../../specification/quirks.md`](../../specification/quirks.md).

| Observation | Classification | Original Mode |
|---|---|---|
| `CREGEN` increments a count and does not birth | source behaviour | preserve; the creature appears on the next `NEWLVL` |
| Opening lap runs `CREGEN` because tasks start in `Q.SCD` | source behaviour | preserve |
| (0, 0) is unusable as a birth cell | source behaviour, a consequence of marking the block in use before storing the position | preserve |
| Generic shield/sword/torch parameters until reveal | source behaviour | preserve |
| `CMOVE` is not queued (D-6) | deviation of this core | retire when movement is implemented |

## 5. Still open

| Priority | Question | Why it is still open |
|---|---|---|
| Closed for bytes | Is the reconstructed listing the same code as catalog 26-3093? | Yes. Zero differing bytes. [`rom-diff.md`](../../provenance/rom-diff.md). |
| Closed | Does the core count the 377 build interrupts? | Yes. The owner decided on 2026-09-25, for fidelity. Original Mode's scheduler entry is `0:0:6.2.5`. |
| Unresolved | Why `CREGEN`'s five-minute reschedule did not run during five minute-queue scans | §1. The opening lap and the re-entry lap did run. |
| Open | D-1 and D-2 for a task that returns `Q.SCD`, and for a task readied by another task in the same pass | The IRQ's own ready order is §1. |
| High | `CMOVE` priorities and attack | Out of scope for this phase. |
| High | Combat wrapping, door and shield edge cases, manual `CLIMB` wording | Unchanged from Phase 0b. |
| Distribution | Rights for the lexicon and tuning tables | Unchanged. `ledger.md` §4. |

## 6. Gate

Every §1 row has a first divergence, a measured result, or a "not run" reason.
Issue #13 is decided: Original Mode counts the 377 build interrupts. The core
does that. The decision is not waiting.

ROM-observed, from §1: the 377-interrupt build and `DGEN90` at `SECOND` = 6,
including seeds and the 24 positions; the opening `CREGEN` increment of type 5
with no birth; maze bytes identical across the recorded entries; creature
positions not identical on re-entry; the extra creature born on the return
`NEWLVL`; turn, half-step, and `THUD` durations; `SNOISE` leaving `SEED`
alone; blocked `MOVE` adding 7 damage at weight 35; same-interrupt task order
`PLAYER`, then `LUKNEW` when a tenth task expired, then `BURNER`.

`population-entry.txt` is the source-derived file: every row is a chosen
`SECOND`, and the `cregen` line is level 0 at `SECOND` = 1 (type 9, then 25
on re-entry). The ROM-observed level-0 entry is not that file. It is Original
Mode, `SECOND` = 6, clock `0:0:6.2.5`, opening type 5.

Still source-derived: the harness clock (`Game(second)`), including
`population-entry.txt`; D-3, D-5, and D-6; combat, objects, and movement.
The `SECOND` = 0, 1, 7, 30, 59 spins are harness-modified and are not
unmodified ROM behaviour. t3 and t5 are reference-only inputs.

`make all` on 2026-09-25, after the trace regeneration, exited 0. It checks
the core against its fixtures and stored traces. It is not the ROM evidence;
§1 is. The stored traces were regenerated from the recorded first divergence
of the previous baseline: each of `t1`–`t5` diverged at trace line 1, jiffy 0,
`INIT` clock `0:0:1.0.0` / `second=1` versus `0:0:6.2.5` / `second=6`. No
fixture hash changed. `population-entry.txt` was not regenerated.

- `ctest`: 2 tests, 0 failures (`conformance`, `trace_diff_identity`).
- Conformance binary: `PASS: 380 checks, 0 failures`.
- `make verify`: `OK: 17 fixtures, 0 problems`.
- Listing pin: `a94326f00ebb16a106b540c58bc2ccf5f7b66dac`.
- Catalog 26-3093 `.ccc` and the LWTOOLS 4.25 image are both 8192 bytes,
  SHA-256 `35e6a77354dcf1a3048f276824b7a0f9f759115fdd40603664cebfb3a7da6571`,
  and `cmp` reports no differing bytes. The `.ccc` is gitignored.

## 7. Next slice

Issue #13 is applied. The source-backed slice after Phase 1 is `CMOVE` only,
retiring D-6, still without attacks. This close-out does not implement it.
The prompt was [`../../prompts/phase-1-apply-issue-13.md`](../../prompts/phase-1-apply-issue-13.md).
Attacks need the matrix decrement in `PATT40`, which this phase left unread
for implementation.
