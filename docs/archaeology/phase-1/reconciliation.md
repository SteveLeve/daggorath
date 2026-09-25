# Archaeology reconciliation — Phase 1

Phase 1's questions were the ROM equivalence of the pinned listing, the
`CREGEN` birth blocker, and the population behaviour `NEWLVL` actually
performs. Items not investigated stay open.

**ROM.** LWTOOLS 4.25 assembled the pinned listing to the same 8192 bytes as
the Tandy catalog 26-3093 image. The diff is in
[`../../provenance/rom-diff.md`](../../provenance/rom-diff.md). MAME 0.264
(`coco`, BIOS `b12e11`) was invoked on that image and exited before a frame:
Color BASIC 1.2 and Extended Color BASIC 1.1 were not on the machine and were
not downloaded. Nothing below is ROM-observed behaviour. "Resolved" means
resolved against the listing at `a94326f`, whose assembled bytes match that
image.

Confidence: **High** = read off the listing and cross-checked by the Python
extractor and the C++ core; **Medium** = read off the listing, one
implementation; **Low** = inference.

## 1. ROM captures

The harness is `tools/rom/assemble.sh`, `tools/rom/capture.lua`,
`tools/rom/watchlist.tsv`, and `tools/rom/trace_diff.py`. `assemble.sh` produced
an 8192-byte image; see `rom-diff.md`. `capture.lua` was run only as
`DOD_SELFTEST=1` (parsers and key table). Its frame loop has not run, because
MAME exited before a frame. `trace_diff.py` was run against `dcli` traces, not
against a ROM trace. The cells below stay “not captured”: there is no ROM
trace, so there is no first divergence and no “none”.

| Capture | First divergence |
|---|---|
| `t1-move-turn-look` | not captured |
| `t2-forward-corridor` | not captured |
| `t3-burst-one-jiffy` | not captured |
| `t4-parser-edges` | not captured |
| `t5-keyboard-overrun` | not captured |
| Level entry at `SECOND` = 0, 1, 7, 30, 59 | not captured |
| Level re-entry | not captured |
| Five-minute `CREGEN` boundary | not captured |
| Simultaneous expirations | not captured |
| MOVE/TURN/LOOK timing (D-4) | not captured |
| Blocked MOVE exertion | not captured |
| `SNOISE` and sound duration | not captured |

D-1 through D-5 are unchanged. D-4's durations were not measured. D-6 is new
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
| High | Animation and sound durations, and whether `SNOISE` consumes `SEED` | No capture. D-4 stands. |
| High | D-1 and D-2 against a running ROM | No capture. |
| High | `CMOVE` priorities and attack | Out of scope for this phase. |
| High | Combat wrapping, door and shield edge cases, manual `CLIMB` wording | Unchanged from Phase 0b. |
| Distribution | Rights for the lexicon and tuning tables | Unchanged. `ledger.md` §4. |

## 6. Gate

The prompt's completion gate is **not met**. Steps 4–6 are done. The cartridge
byte diff is done and has zero differences. Step 3 is not: MAME 0.264 exited
before a frame, for the reason in
[`rom-diff.md`](../../provenance/rom-diff.md), so the "not captured" rows in
§1 stand. Behavioural claims stay source-derived.

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
