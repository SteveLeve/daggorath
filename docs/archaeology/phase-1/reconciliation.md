# Archaeology reconciliation — Phase 1

Phase 1's questions were the ROM equivalence of the pinned listing, the
`CREGEN` birth blocker, and the population behaviour `NEWLVL` actually
performs. Items not investigated stay open.

**ROM.** LWTOOLS 4.25 assembled the pinned listing to the same 8192 bytes as
the Tandy catalog 26-3093 image. The diff is in
[`../../provenance/rom-diff.md`](../../provenance/rom-diff.md). On 2026-09-25
MAME 0.264 `coco2b` (Color BASIC 1.3, Extended Color BASIC 1.1, matching the
owner's CoCo 2B; rights basis in the ledger) ran that image and the five
Phase 0b keystroke scripts. §1 holds those results. Everything else below
is still resolved against the listing at `a94326f`, not ROM-observed.

Confidence: **High** = read off the listing and cross-checked by the Python
extractor and the C++ core; **Medium** = read off the listing, one
implementation; **Low** = inference.

## 1. ROM captures

The harness is `tools/rom/assemble.sh`, `tools/rom/capture.lua`,
`tools/rom/watchlist.tsv`, `tools/rom/trace_diff.py` and
`tools/rom/run-capture.sh`. Raw samples and traces are in gitignored
`captures/`. Two runs of all five scripts gave byte-identical raw files.

**Alignment (inferred).** The cartridge boots into the autoplay demo
(`ONCE.ASM DEMO`). The harness holds SPACE until the demo's `CLOCK` transfers
to `GAME` (`COMMON.ASM` CLK50 autoplay abort). Jiffy 0 is the first `CLOCK`
after `GAME50` is fetched, i.e. scheduler entry, where the reference slice
emits `INIT`. Keys are injected at the PIA ($FF02 strobe, $FF00 rows), so a
script key for jiffy N is down for interrupt N's `POLCAT` scan. The ROM trace
reports only state visible in RAM (`PDIR`, `PROW`/`PCOL`, `PDAM`), at the
jiffy the change is first seen; the reference reports events at dispatch. A
ROM trace therefore has no `LINE`, `OUTPUT`, `LOOK` or blocked-`MOVE` lines,
and its `EXERT` lines include recovery ticks the reference does not print.

| Capture | First divergence (`trace_diff.py`) |
|---|---|
| `t1-move-turn-look` | line 1, jiffy 0: `INIT` clock `0:0:1.0.0` (reference) vs `0:0:6.2.5` (ROM) |
| `t2-forward-corridor` | same line and values as t1 |
| `t3-burst-one-jiffy` | same line and values as t1 |
| `t4-parser-edges` | same line and values as t1 |
| `t5-keyboard-overrun` | same line and values as t1 |
| Level entry at `SECOND` = 0, 1, 7, 30, 59 | not captured: no script yet |
| Level re-entry | not captured: no script yet |
| Five-minute `CREGEN` boundary | not captured: no script yet |
| Simultaneous expirations | not captured: needs the `SCHED_JSR` task log |
| MOVE/TURN/LOOK timing (D-4) | partial, see below |
| Blocked MOVE exertion | partial, see below |
| `SNOISE` and sound duration | not captured: no DAC log yet |

Observations past `INIT` (reference jiffy at dispatch → ROM jiffy the state
changed):

| Script | Event | Reference | ROM |
|---|---|---|---|
| all | level 0 build, `GAME10` IRQSYN to `GAME50` | not modelled | 377 interrupts; clock `0:0:6.2.5` at scheduler entry |
| t1 | `T R` → `PDIR` E | 9 | 16 |
| t1 | blocked `M` → `PDAM` 7, `HEARTR` 40 | 32 | 52 (`PDAM` 7, `HEARTR` 40) |
| t1 | blocked `M` → `PDAM` 14, `HEARTR` 36 | 62 | 82 (`PDAM` 14, `HEARTR` 36) |
| t1 | `T A` → `PDIR` W | 94 | 95 |
| t2 | `M` → (15,11), `PDAM` 7, `HEARTR` 40 | 7 | position 20, `PDAM` 24 |
| t2 | `M` → (14,11) | 42, `PDAM` 14 | position 47, `PDAM` 13 at 50 (recovery tick at 25) |
| t2 | `M B` → (15,11) | 79, `PDAM` 21 | position 84, `PDAM` 19 at 88 |
| t4 | `Z`, `MOVEX`, `T` | `???`, no state change | no state change |
| t4 | `M` → (15,11) | 57 | position 63, `PDAM` 67 |
| t3, t5 | burst in one jiffy | dispatched | no state change: all keys held at once for one scan |

Interpretation, **Low** confidence until the task log exists:

- The reference starts the clock at `0:0:1.0.0` at scheduler entry. On the ROM
  the clock has run through the level-0 build. This is a real difference in
  initial state, and it shifts the phase of every clock-driven task.
- Blocked MOVE still adds exertion on the ROM (`PDAM` 7 then 14 with no
  position change). This matches the source reading and the core.
- Heart rate and damage values match the reference wherever no recovery tick
  intervenes. The later `PDAM` differences in t2 are recovery ticks at a
  different clock phase, not a different exertion formula.
- The delay between keystroke and state change is 1 jiffy for `T A`, 7 for
  `T R` and 5 to 13 for `MOVE`. That is the turn sweep and half-step
  animation cost D-4 leaves open, but dispatch time is not yet observed on the
  ROM, so these are upper bounds, not durations.
- §2 and `population-entry.txt` take level 0 as entered at `SECOND = 1`. On the
  ROM the opening `CREGEN` lap runs at scheduler entry, with `SECOND` = 6, and
  `DGEN90` runs somewhere inside the 377-interrupt build. Neither value has
  been checked against `SEED`/`CMXLND` samples yet. No fixture was changed.
- t3 and t5 are not replayable on the hardware. `POLCAT` returns at most one
  key per interrupt, so "several keys in one jiffy" is a reference-only input.

D-1 through D-5 are unchanged. D-4 has upper bounds only (above). D-6 is new
and is a deviation of the core, not a ROM finding.

The committed Phase 0b traces were replayed with the extended core. They
match byte for byte, because `CREGEN` does not emit a trace line and `CMOVE`
is not queued.

## 2. How a matrix increment becomes a creature

| Question | Result | Confidence |
|---|---|---|
| Does `CREGEN` create a control block? | **No.** It increments `CMXLND[LEVEL][(RANDOM & 7) + 2]` when the 8-bit sum of that row is below 32, then reschedules at 5 minutes. | High |
| When does the extra creature exist? | The next `NEWLVL` for that level. `NEWLVL` zeros the control blocks and calls `CBIRTH` once per current matrix count. The matrix is not recopied from `CMTTAB`. | High |
| Does this include returning to a level? | Yes. `CMXLND` survives the level change. Time on another level does not increment this row, because `CREGEN` uses `CMXPTR`. | High |
| Why does one increment happen before five minutes? | `SYSTCB` puts every system task in `Q.SCD` with countdown 0, so the opening lap runs `CREGEN` once. On level 0 at `SECOND = 1` that draw increments type 9. Live count stays 24 until re-entry, which births 25. | High |

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
| Closed for bytes | Is the reconstructed listing the same code as catalog 26-3093? | Yes. Zero differing bytes. [`rom-diff.md`](../../provenance/rom-diff.md). Runtime behaviour is still unmeasured. |
| High | Animation and sound durations, and whether `SNOISE` consumes `SEED` | D-4 has upper bounds from §1; dispatch time and the DAC log are not captured. |
| High | D-1 and D-2 against a running ROM | Needs the `SCHED_JSR` task log, not yet in `capture.lua`. |
| High | `CMOVE` priorities and attack | Out of scope for this phase. |
| High | Combat wrapping, door and shield edge cases, manual `CLIMB` wording | Unchanged from Phase 0b. |
| Distribution | Rights for the lexicon and tuning tables | Unchanged. `ledger.md` §4. |

## 6. Gate

The prompt's completion gate is **not met**. Steps 4–6 are done. The cartridge
byte diff is done and has zero differences. Step 3 is partial: the five scripts
ran on the ROM and §1 records their first divergences, but seven priority
captures are still "not captured". Behavioural claims stay source-derived
except where §1 names a ROM observation.

`make all` on 2026-09-25 exited 0. That run checks the core against its own
fixtures and traces. It is not ROM evidence.

- Listing pin: `a94326f00ebb16a106b540c58bc2ccf5f7b66dac`.
- `ctest`: 2 tests, 0 failures (`conformance`, `trace_diff_identity`).
- Conformance binary: `PASS: 360 checks, 0 failures`.
- `make verify`: `OK: 17 fixtures, 0 problems`.
- Each of the five Phase 0b scripts, replayed through `dcli` for 200 jiffies
  at `SECOND = 1` and compared with `trace_diff.py` to the stored trace:
  `no divergence` (446, 442, 435, 442, and 436 lines).
- Catalog 26-3093 `.ccc` and the LWTOOLS 4.25 image are both 8192 bytes,
  SHA-256 `35e6a77354dcf1a3048f276824b7a0f9f759115fdd40603664cebfb3a7da6571`,
  and `cmp` reports no differing bytes. The `.ccc` is gitignored.

## 7. Next slice

The cartridge bytes match. A frame capture is still required before any
source-proven claim is treated as ROM-observed. It is blocked on a legally
obtained Color BASIC 1.2 and Extended Color BASIC 1.1 image, as recorded in
[`rom-diff.md`](../../provenance/rom-diff.md).

The next gameplay slice the source supports is creature movement (`CMOVE`
only, retiring D-6), still without attacks. Attacks need the matrix decrement
in `PATT40`, which this phase left unread for implementation.
