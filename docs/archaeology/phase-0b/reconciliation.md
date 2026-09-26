# Archaeology reconciliation — Phase 0b

For every `VERIFY` item and open question in `Phase-0-Archaeology.md` that this
phase investigated: the evidence, the result, the confidence, and whether a
source-versus-ROM difference is possible. Items not investigated are listed in §3
and remain open, untouched.

**Global caveat:** no retail ROM or emulator was available (see
[`../../provenance/ledger.md`](../../provenance/ledger.md) §5). Every "resolved" row below is resolved **against the
reconstructed listing at `a94326f`**, which carries 2022 `lwasm` edits. Nothing
here is ROM-verified.

Confidence scale: **High** = read directly off the listing and cross-checked by
two independent implementations; **Medium** = read off the listing, single
implementation or partially inferred; **Low** = inference.

---

## 1. Resolved against the reconstructed source

### 1.1 Scheduler and clock (§4–5, §21 blocker 1)

| Question | Result | Confidence |
|---|---|---|
| Exact `ROLTAB` values | 6, 10, 60, 60, 24 (`COMMON.ASM ROLTAB`). 1 tenth = 6 jiffies, 1 second = 60, 1 minute = 3600. `DAY` never rolls over. | High |
| FIFO insertion under simultaneous expirations | `QUEADD` appends at the tail; `QUESCN` walks each queue in list order; `CLK42` walks queues in ascending time unit. So ties resolve jiffy → tenth → second → minute → hour, FIFO within a queue. | High |
| Does all ready work finish within a jiffy? | **No, and the question does not apply.** `SCHED` is an endless loop that restarts at the head when it reaches the tail; there is no per-jiffy boundary in the foreground. Time advances underneath a running task via the interrupt. | High |
| Blocking `SYNC`/`WAIT` | `SYNC` blocks the whole foreground until the next interrupt. `PTURN`/`PMOVE` end in `DEC UPDATE / SYNC`, so a command's frame is presented by the following interrupt. `WAITX` is 81 × `SYNC` ≈ 1.35 s despite its "1.5 second" comment. | High |
| Keyboard queue overflow | **No overflow check exists.** `KBDPUT` writes unconditionally into a 32-byte circular buffer; the source comment says so. A 32-character burst leaves head == tail, i.e. the buffer reads as *empty*. Silent corruption, no error. | High |
| Sound duration / whether sound blocks | Sound generation runs in the foreground, so it does block. Exact durations are **not** derivable from the listing. | Medium (that it blocks) / open (durations) |

**Correction to Phase 0.** The report states "`PLAYER` gets at most one buffered
character on its turn". That is wrong. `PLAY10` loops `KBDGET` until it returns
null and processes every character, so a whole command typed within one jiffy is
echoed, parsed and dispatched inside a single `PLAYER` turn. Verified by the
reference slice (`t3-burst-one-jiffy`). The *interrupt* side is what is limited to
one character per jiffy.

**Correction to Phase 0.** The report treats faint as "keyboard scanning is
suspended". It is stronger: the interrupt skips the poll **and** `PLAYER`
discards whatever was already buffered.

**New finding.** When `LINBUF` fills at 32 characters, `HMAN20` falls through the
buffer-full test straight into the carriage-return path, so an over-long line is
dispatched without the player pressing return.

### 1.2 Maze generation and movement (§7, §21 "High" row)

| Question | Result | Confidence |
|---|---|---|
| Are the five maps fixed, and identical at every entry time and on re-entry? | **Yes, in the source.** `DGNGEN` seeds from `LVLTAB + LEVEL` and `DGEN90` draws only *after* the maze and all 115 doors are complete. Generated at `SECOND` = 0, 1, 7, 30, 59 and on repeat entry, all five levels produce byte-identical 1024-byte arrays (`fixtures/mazes.json`, `entry_time_invariance`). | High (source); **unverified against ROM** |
| `LVLTAB+LEVEL` overlapping windows | Confirmed: the table is 7 bytes `73 C7 5D 97 F3 13 87` and level *n* uses bytes *n*, *n+1*, *n+2*. The commented-out `$25` debug byte is **not** in the table. | High |
| Which later placements depend on `SECOND`? | Everything drawn after the maze. The pre-`DGEN90` seed is identical across entry times; the post-spin seed differs. Creature placement in `NEWLVL` therefore varies with entry time while the map does not. | High |
| `RNDCEL` draw order | **The first draw is the column, the second is the row.** `RNDCEL` does `RANDOM → AND 31 → TFR A,B` (column into B) and then a second `RANDOM → AND 31` for the row, falling into `MAP32`. Getting this backwards produces a plausible but wrong maze. | High |
| Exact corner rejection arithmetic | Four three-byte sums over the 3×3 neighbourhood at offsets (3,0,1), (1,2,5), (5,8,7), (7,6,3), each rejected when the 8-bit sum is zero. Since cells hold only `$00` or `$FF` at that point, a zero sum means all three are already carved — the test rejects creating a 2×2 open block. | High |
| Map door traversal semantics | **Doors do not block movement, and neither do walls between carved cells.** `STEPOK` rejects only out-of-bounds and the never-carved pattern `$FF`; there is no per-edge test. This is consistent rather than buggy: Phase II raises a wall bit only toward an `$FF` neighbour, and `MAKDOR` places a door only where the edge is already a passage. Verified: all 500 carved cells on level 0 are reachable from the start cell under `STEPOK`. | High |
| `DGEN90` with `SECOND = 0` | Spins **256** times (`LDB SECOND / DEC B / BNE`), not zero. A naive `for (i = 0; i < second; ++i)` loses this. | High |
| Vertical feature destinations | `VFTTAB` bytes extracted verbatim (`fixtures/vertical-features.json`); records are `kind, row, col` with kind 1 = ladder, 0 = hole. The **grouping of records to levels** via the `$80` terminators is inferred from the source's comment columns and is **not** resolved. | Medium (bytes) / open (level mapping) |

Maze serialization is now pinned: 1024 bytes, row-major, index `row*32 + col`, one
byte per cell, bit pairs low-to-high North/East/South/West, `00` passage, `01`
regular door, `10` secret door, `11` wall. Hashes are SHA-256 over those bytes in
that order. Changing the serialization changes every hash, so the definition is
part of the fixture contract.

### 1.3 Parser and lexicon (§8, §21 blocker 3)

| Question | Result | Confidence |
|---|---|---|
| Exact parser aliases | Derived, not guessed. All four token tables were decoded from `TOKEN.ASM`'s packed 5-bit strings and the shortest unique prefix computed under `PARSER`'s rule (`fixtures/parser-prefixes.json`). Every command has a one-letter abbreviation except `ZLOAD`/`ZSAVE`, which need two (`ZL`, `ZS`); a bare `Z` is ambiguous and prints `???`. | High |
| Ambiguity versus exact match | An exact full-word match does **not** win over an ambiguity. `PARSER` checks `PARFLG` (already seen a match) before any full-word preference, so a word that is also a prefix of another entry fails. In the shipped tables no entry is a prefix of another, so this case is currently unreachable — but the rule matters for any added vocabulary. | High |
| Trailing tokens | Ignored. Each handler parses only the tokens it needs; the rest is discarded when `LINPTR` resets. No error. | High |
| Overlong input | Dispatched at 32 characters without a return (see §1.1). | High |
| `BACKWARD` versus `BACK` | **The table entry is `BACK`.** `DIRXXX` in `DTABAS.ASM` names the macro argument `BACKWARD`, but the packed string in `TOKEN.ASM` decodes to `BACK` (4 letters). So `BACKWARD` is **rejected** — it is longer than the entry and cannot be a prefix of it. Phase 0 had this backwards: it says `DIRXXX` spells `BACKWARD` and prefix matching accepts the manual's `BACK`. | High |
| Full ring incantations | The typed adjectives come from `ADJTAB`, not from the `OBJXXX` macro's label. Ring words are `VULCAN`, `RIME`, `JOULE`, `SUPREME`, transforming to `FIRE`, `ICE`, `ENERGY`, `FINAL` (plus `GOLD`), matched through `XXXTAB`'s `T.RN12`/`T.RN13`/`T.RN11`/`T.RN15` parameters. | High |

**Correction to Phase 0.** The report lists ring labels "VULCAN/HOTH/JOULE/SUPREME".
`HOTH` is the `OBJXXX` macro label for that object; the **player-facing word is
`RIME`**. The two tables are positionally parallel and differ at exactly that one
entry. A port that ships `HOTH` as a typed word gets it wrong.

### 1.4 Player physiology and items (§10–12)

| Question | Result | Confidence |
|---|---|---|
| Blocked MOVE still costs exertion | Confirmed: every `PMOVE` path, including the `THUD` path, falls through `PMOV90`, which adds `(POBJWT >> 3) + 3` to `PDAM` and calls `HUPDAT`. Regression-tested. | High |
| Heart-rate formula | `HEARTR = (P*64)/(P + 2D) − 19`, 24-bit, where the division loop **increments the quotient before testing the borrow**, so it yields `floor(num/den) + 1`. Stored in one byte, compared signed. Faint at `≤ 3`, recover above `4`. | High |
| Starting power | **`PPOW = 160`, not 0.** `RAMDAT` presets `$17A0`; `GAME10`'s `CLR PPOW` clears only the high byte of the two-byte field. Starting `POBJWT = 35` matches the initial bag exactly (wooden sword 25 + pine torch 10). Starting `HEARTR` is therefore 46 jiffies. Reading `CLR PPOW` as a full clear makes the player faint on the first interrupt — which is how this was caught. | High |
| Why the dead-torch threshold is five | `BURNER` marks the torch dead when the timer is `≤ 5`, and then clamps regular and magical light to the remaining timer value. So a torch is *displayed* as dead for its last five minutes while its light continues to decay to zero. The nominal 15/30/60-minute manual figures are the `XXXTAB` timer values 15/30/60 for pine/lunar/solar. | High |
| Exact end-of-light curve | Light is `min(current light, remaining timer)`, updated once per minute, never raised. | Medium |

## 2. Newly documented quirks

| Observation | Classification | Action for Original Mode |
|---|---|---|
| `PLAYER` drains the whole keyboard buffer per turn | source behaviour, intentional | preserve; timestamp keystrokes individually |
| 32-byte keyboard buffer with no overflow check | source behaviour, probably an accepted limitation | preserve; document as observable |
| `LINBUF` full dispatches the line without a return | source behaviour, likely intentional | preserve; regression-tested |
| `CLR PPOW` clears one byte of a two-byte field | source behaviour, deliberate ("correct power settings") | preserve; starting power is 160 |
| `LUKNEW` comment says "twice per second", code says 3 tenths | comment error | follow the code (0.3 s) |
| `WAITX` comment says 1.5 s, code is 81 jiffies (1.35 s) | comment error | follow the code |
| `MAKDOR` retries until it finds an edge whose 2-bit field is `00`, so an edge never ends up holding both a regular and a secret door, and exactly 70 + 45 doors are placed | source behaviour, verified | preserve the retry loop: it consumes RNG draws and so affects the post-maze stream |
| `DGEN90` spins 256 times at `SECOND = 0` | source behaviour | preserve explicitly |
| Movement legality ignores edge bits entirely | source behaviour, self-consistent | preserve; do **not** "fix" by adding a wall test |
| `ADJTAB` word for the `HOTH` object is `RIME` | documentation/source naming difference | ship `RIME` |
| `DIRTAB` entry is `BACK`, so `BACKWARD` is rejected | source/manual discrepancy resolved in favour of `BACK` | ship `BACK`; confirm the manual's wording against a ROM probe |

## 3. Still open

| Priority | Question | Why it is still open |
|---|---|---|
| Blocker | Is the reconstructed listing behaviour-equivalent to the retail ROM? | No ROM and no `lwasm` run. Every row in §1 inherits this uncertainty. |
| Blocker | How does a `CREGEN` matrix increment become a live creature, including on level re-entry? | Requires tracing `COMCRE`'s creation path plus a five-minute emulator observation. Not investigated this phase. |
| High | Animation and sound durations in jiffies | Not derivable from the listing; needs ROM timing (spec deviation D-4). |
| High | Does `SOUNDS:SNOISE` consume `SEED`, and the exact distance→volume mapping | `SOUNDS.ASM` not analysed this phase. |
| High | `VFTTAB` group-to-level mapping | Bytes extracted; the `$80` terminator grouping produces fewer non-empty groups than five, so the mapping needs a `PCLIMB`/`COMCRE` walkthrough. |
| High | Combat byte wrapping and signed flags (`ATTACK`, `DAMAGE`, `SCAL16`) | Deliberately out of scope for Phase 0b. |
| High | Door, darkness, shield-filter and 16-bit overflow edge cases | Out of scope. |
| High | Is the manual's bare `CLIMB` inconsistent with the game? | Needs a ROM probe; `PCLIMB` not analysed this phase. |
| Medium | Whether a task returning `Q.SCD` is re-entered within the same jiffy in practice | Spec deviation D-2; measurable only against a ROM trace. |
| Distribution | Rights for the extracted lexicon and tuning tables | See [`../../provenance/ledger.md`](../../provenance/ledger.md) §4. Needs counsel. |

## 4. Phase 1 pointer

Phase 1's reconciliation is
[`../phase-1/reconciliation.md`](../phase-1/reconciliation.md). It does not
edit the sections above. Phase 1 counts the 377 level-0 build interrupts;
the reference traces open at `0:0:6.2.5`.

The quirks ledger for Phase 0b is §2 of this file and is left as dated. The
canonical ledger, which includes those rows and the Phase 1 rows, is
[`../../specification/quirks.md`](../../specification/quirks.md). Deviation D-6
is in [`../../specification/clock-and-scheduler.md`](../../specification/clock-and-scheduler.md)
§13. The `CREGEN` blocker and the `VFTTAB` mapping row in §3 above are answered
in that Phase 1 document; this table stays the Phase 0b record.

Two fixture facts belong here because `make verify` requires the reason beside
this pack:

- `vertical-features.json` gained `newlvl_vftptr_byte_index`, computed by the
  `NEWLVL` walk. The raw bytes and the grouped records are the same.
- `population.json` and `population-entry.txt` were added. No existing fixture
  hash was replaced to make a comparison pass.

The ROM measurement was not made. The record is
[`../../provenance/rom-diff.md`](../../provenance/rom-diff.md), not a second
copy of that text.
