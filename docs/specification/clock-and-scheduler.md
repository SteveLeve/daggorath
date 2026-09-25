# Clock and scheduler specification (jiffy-by-jiffy)

**Phase 0b deliverable (c).** Evidence base: reconstructed 1983 assembly listing at
commit `a94326f`, files `COMMON.ASM`, `CD.ASM`, `COMDAT.ASM`, `ONCE.ASM`,
`HUMAN.ASM`, `COMPLR.ASM`, `HUPDAT.ASM`, `COMCRE.ASM`, `PTURN.ASM`, `MISC.ASM`.
No retail ROM was available in this environment, so **no rule below is
ROM-observed**; see `traces/README.md`.

Every rule carries one label:

| Label | Meaning |
|---|---|
| **[SRC]** | source-proven: the rule is read directly off the listing |
| **[ROM]** | ROM-observed: confirmed against a retail ROM under an emulator |
| **[INF]** | inferred: a reading the listing supports but does not state |
| **[OPEN]** | unresolved: needs ROM tracing or further source work |

No rule in this document is currently labelled **[ROM]**.

---

## 1. Time base and counters

| Item | Value | Label | Evidence |
|---|---|---|---|
| Interrupt source | PIA #1 horizontal-sync ("jiffy") IRQ, 60 Hz | [SRC] | `COMMON.ASM CLOCK`; `ONCE.ASM IRQSYN` sets IRQ on the falling edge of FS |
| Counter chain | `JIFFY → TENTH → SECOND → MINUTE → HOUR → DAY` | [SRC] | `COMMON.ASM CLK42`, `CD.ASM` |
| Rollovers (`ROLTAB`) | 6, 10, 60, 60, 24 | [SRC] | `COMMON.ASM ROLTAB` |
| Therefore | 1 tenth = 6 jiffies; 1 second = 60 jiffies; 1 minute = 3600 jiffies | [SRC] | derived from `ROLTAB` |
| `DAY` | incremented, never rolled over | [SRC] | `CLK42` stops at `CMPX #DAY` |

A "jiffy" in this document means one 1/60 s interrupt, i.e. one `CLOCK`
invocation. `JIFFY` the *counter* is a 0..5 sub-tenth counter, not a free-running
tick count.

## 2. Interrupt order inside one jiffy

`CLOCK` runs in exactly this order **[SRC]** (`COMMON.ASM CLOCK`):

1. **Reject non-jiffy interrupts.** `LDA P.PICRB-$20,X / LBPL CLK90` — anything
   that is not the jiffy interrupt is dismissed without any of the work below.
2. **Set DP to `$0200`.**
3. **Display swap** (`CLK20` entry path): if `UPDATE` is non-zero, exchange
   `FLIP`/`FLOP`, reprogram the SAM page, clear `UPDATE`. A command that ends in
   `DEC UPDATE / SYNC` therefore has its new frame presented by the *next*
   interrupt.
4. **Opening buzz**: if `NOISEF`, complement `NOISEV` and write it to the DAC.
5. **Heartbeat**: if `HBEATF`, decrement `HEARTC`; on reaching zero reload it
   from `HEARTR`, toggle the single-bit sound output, and — only if `HEARTF` —
   redraw the heart glyph on the status line. Audible and visible heartbeat are
   separately gated: `HEARTF` is the *map-mode* flag, so audio continues while
   the map suppresses the glyph.
6. **`QUESCN(JIFQUE)`** — the jiffy countdown queue is scanned on *every*
   interrupt, before the counters are bumped.
7. **Counter bump with cascading queue scans** (`CLK42`): increment `JIFFY`; if it
   has reached its `ROLTAB` value, clear it, advance to the next queue pointer and
   `QUESCN` that queue, then repeat for `TENTH`, `SECOND`, `MINUTE`, `HOUR`. The
   loop stops at the first counter that has not rolled over. `DAY` terminates it.
8. **Keyboard poll** (`CLK50`): if `FAINT` is non-zero, **skip the poll entirely**
   — keystrokes during a faint are never buffered, not merely ignored later.
   Otherwise `POLCAT` is called and a non-null character is deposited with
   `KBDPUT`. At most one character enters the buffer per interrupt **[SRC]**.
9. **Reset the PIA and `RTI`.**

### 2.1 Consequences worth preserving

- The jiffy queue is scanned once per interrupt; the tenth queue is scanned once
  per 6 interrupts, at the same instant the tenth counter rolls, and so on. A
  minute-queue task and a jiffy-queue task that both expire on the same interrupt
  are handled in **queue order: jiffy first, then tenth, second, minute, hour**
  **[SRC]**, because `CLK42` walks the queues in ascending time unit.
- A task whose countdown expires is *not* run inside the interrupt. It is only
  appended to the ready queue; execution happens in the foreground **[SRC]**.

## 3. Countdown queues (`QUESCN`)

**[SRC]** `COMMON.ASM QUESCN`:

1. If `SLEEP` is non-zero, return immediately — no queue is processed and no
   countdown advances. (`SLEEP` is used around cassette save/load.)
2. Walk the queue's singly-linked TCB list from its head.
3. For each TCB: `DEC P.TCTIM`. If the result is non-zero, continue to the next
   TCB.
4. If it reached zero: `QUERMV` the TCB from this list and `QUEADD` it with code
   `Q.SCD`.
5. Continue traversal from the removed node's predecessor, so no TCB is skipped.

**Simultaneous expirations within one queue** are appended to the ready queue in
**list-traversal order** **[SRC]**, and `QUEADD` always appends at the tail
(`QUEAD1` walks to the null link), so the ready queue is strictly FIFO **[SRC]**.

List order within a countdown queue is the order in which TCBs were added to that
queue, which for the system tasks is the `TCBDAT` order at startup and thereafter
the order in which each task happened to be rescheduled. A port must therefore
keep an ordered list, not a set or a priority heap **[INF]**.

## 4. TCB layout and queue codes

**[SRC]** `CD.ASM`:

| Field | Offset |
|---|---|
| `P.TCPTR` link | 0 (2 bytes) |
| `P.TCTIM` countdown | 2 (1 byte) |
| `P.TCRTN` routine | 3 (2 bytes) |
| `P.TCDTA` data block | 5 (2 bytes) |
| `TC.LEN` | 7 |

Queue codes are byte offsets into the queue-pointer table: `Q.NUL 0`, `Q.JIF 2`,
`Q.TEN 4`, `Q.SEC 6`, `Q.MIN 8`, `Q.HOU 10`, `Q.SCD 12`.

A countdown is **one byte**, so the longest single scheduling delay is 255 units
of the chosen queue, and a countdown of 0 would never expire (`DEC` from 0 wraps
to 255) **[INF]**.

`GETTCB` is a bump allocator over a fixed TCB arena and never frees **[SRC]**;
a destroyed task's storage is reclaimed only by `SYSTCB` resetting `TCBPTR`.

## 5. Foreground dispatch (`SCHED`)

**[SRC]** `COMMON.ASM SCHED`:

```
SCHED:  U := &SCDQUE ; RSTART := 0
SCHED0: Y := U                       ; Y is the previous node
SCHED1: if ZFLAG > 0 -> SAVE ; if ZFLAG < 0 -> LOAD
        U := U->link
        if U == null -> restart at SCHED (head of list)
        call U->routine               ; task runs to completion, B = next queue
        if RSTART != 0 -> restart at SCHED
        if B == Q.SCD -> goto SCHED0  ; task stays ready
        QUERMV ; QUEADD(B, A)         ; A = countdown returned by the task
        U := Y ; goto SCHED1
```

Key properties:

- `SCHED` is an **endless loop**, not a per-jiffy pass. When it reaches the tail
  it starts again at the head. There is no notion of "all ready work finished
  within a jiffy" **[SRC]** — this answers the Phase 0 open question directly:
  ready work is not bounded by the jiffy, and the interrupt is what advances time
  underneath a task that is still running.
- Tasks are **cooperative and non-preemptible** by other tasks. Only the
  interrupt runs in between.
- A task that returns `Q.SCD` remains in the ready list and will be re-entered on
  the next lap. `PLAYER` avoids that by returning `Q.JIF, 1`.
- `RSTART` lets a task force the walk back to the head, e.g. after it has
  destroyed TCBs (level change).
- Cassette save/load is tested *inside* the loop, so `ZSAVE`/`ZLOAD` take effect
  between tasks rather than inside one **[SRC]**.

### 5.1 Blocking inside a task

`SYNC` (wait for the next interrupt) and `WAIT` are executed **inside** the task,
which blocks the whole foreground **[SRC]**:

- `PTURN` and `PMOVE` end with `DEC UPDATE / SYNC`: one jiffy is consumed after
  the command's state change, and the new frame appears on that interrupt
  (`PTURN.ASM PTUR90`, `PMOV90`).
- `WAITX` (`MISC.ASM`) is `81 × SYNC` ≈ 1.35 s of blocked foreground, despite its
  "1.5 second" comment.
- Turn and sidestep animation (`LRTURN`/`RLTURN`) draws 8 lines per sweep with
  `VECTOR` and no `SYNC`, so its cost is CPU-bound and not expressible in jiffies
  from the listing alone **[OPEN]**.
- Sound generation runs waveform loops in the foreground (`SOUNDS.ASM`), so a
  long sound delays every other task while the clock keeps running **[SRC]**;
  the exact durations are **[OPEN]**.

## 6. Startup task set

**[SRC]** `COMDAT.ASM TCBDAT` and `ONCE.ASM SYSTCB`: five TCBs are created, all
added with `LDD #Q.SCD` — countdown 0, queue `Q.SCD` — in this order:

1. `PLAYER` — keyboard input and command dispatch
2. `LUKNEW` — delayed screen update
3. `HSLOW` — damage recovery / heart-rate hysteresis
4. `BURNER` — torch burn
5. `CREGEN` — creature regeneration

So the very first foreground lap runs them in exactly that order, and that order
also seeds the FIFO behaviour of every later tie.

Reschedule intervals **[SRC]**:

| Task | Returns | Notes |
|---|---|---|
| `PLAYER` | `SCHED$ 1,Q.JIF` | every jiffy |
| `LUKNEW` | `SCHED$ 3,Q.TEN` | 3 tenths = 0.3 s, although the source comment says "twice per second" |
| `HSLOW` | `A := HEARTR`, `B := Q.JIF` | interval follows the heart rate |
| `BURNER` | `SCHED$ 1,Q.MIN` | every minute |
| `CREGEN` | `SCHED$ 5,Q.MIN` | every five minutes |

Creature tasks are inserted into `Q.TEN` (`COMCRE.ASM`), and creature delays in
`CDBTAB` are therefore **tenths of a second** **[SRC]**.

## 7. Keyboard path

**[SRC]** `COMMON.ASM KBDPUT`/`KBDGET`, `HUMAN.ASM PLAYER`/`HUMAN`, `CD.ASM`.

1. **Buffer**: 32-byte circular buffer, head `KBDHDR`, tail `KBDTAL`, both masked
   with `$1F`. **There is no overflow check** — the source says so explicitly and
   `KBDPUT` writes unconditionally. A burst longer than 32 characters overwrites
   unread characters, and a burst of exactly 32 leaves head == tail, i.e. an
   apparently *empty* buffer. This resolves the Phase 0 "keyboard queue overflow"
   question: silent corruption, no error, no back-pressure **[SRC]**.
2. **Fill rate**: at most one character per interrupt (`CLK60`), so the buffer
   cannot overflow from the keyboard alone in under 32 jiffies; it can overflow
   whenever the foreground is blocked (long sound, `WAITX`, animation) **[INF]**.
3. **Drain**: `PLAYER` loops `KBDGET` until it returns null — it consumes the
   **entire** buffer on its turn, not one character. A whole command typed inside
   one jiffy is therefore echoed, parsed and executed within a single `PLAYER`
   turn **[SRC]**. (Phase 0 stated one character per turn; that was wrong.)
4. **While fainted**, `PLAYER` still drains the buffer but discards every
   character (`PLAY10: TST FAINT / BNE PLAY10`) **[SRC]**. Combined with the
   interrupt-side skip, a faint both stops new input and throws away whatever was
   already buffered.
5. **Translation** (`PLAY12`/`PLAY14`): space → `I.SP` (`$00`); CR → `I.CR`
   (`$1F`); BS → `I.BS` (`$24`); `A`..`Z` → `ASCII AND $1F`; **anything else
   becomes a space** **[SRC]**. Lower case is not mapped, so a lower-case
   keystroke is not a letter to this game.
6. **Line editing** (`HUMAN`): characters are appended to `LINBUF` (32 bytes,
   `CD.ASM:584`); `I.BS` backs up unless already at the start; on `I.CR` the
   buffer is terminated and dispatched. **When the buffer fills, `HMAN20` falls
   through into the carriage-return path**, so a 32-character line is dispatched
   without the player pressing return **[SRC]**.
7. **Map mode**: if `HEARTF` is zero (map up), the first character received
   forces the forward view back and re-prints the prompt before the character is
   processed **[SRC]**.

## 8. Command dispatch

**[SRC]** `HUMAN.ASM HMAN50`, `PARSER.ASM`, `DTABAS.ASM DISPAT`.

1. `PARSER` is called against `CMDTAB`. Null token → nothing happens, no error.
   Search failure → `CMDERR` prints `???`. Otherwise the token type indexes
   `DISPAT`.
2. `PARSER`'s matching rule: the typed token matches a table entry if it is a
   **prefix** of that entry. If **two** entries match, the result is a failure,
   *even when one of them is an exact full-word match* — `PARFLG` is checked
   before any full-match preference is applied. `FULFLG` records a full-word
   match but only `INCANT` consults it **[SRC]**.
3. Trailing tokens are simply left on the line; each handler parses only what it
   needs and the remainder is discarded when `LINPTR` is reset **[SRC]**.
4. The shipped `CMDTAB` has 15 verbs. `RESTART`, `SETOPT`, `SETCHEAT` and a bare
   `BACK` verb do not exist. The debug commands (`QMAP` and friends) are inside
   `IF DEBFLG` and are absent from a `DEBFLG = 0` build **[SRC]**.

## 9. Physiology timing that affects scheduling

**[SRC]** `HUPDAT.ASM HUPDAX`, `COMPLR.ASM HSLOW`:

- `HEARTR = (P*64)/(P + 2D) − 19`, in 24-bit arithmetic, where the division is a
  repeated-subtraction loop that **increments the quotient before testing the
  borrow**, so it yields `floor(num/den) + 1`. The result is stored in one byte
  and compared **signed**.
- Faint when `HEARTR ≤ 3`; recover when `HEARTR > 4` (hysteresis band at 4).
- `HSLOW` removes `damage/64` per run and then reschedules itself `HEARTR` jiffies
  later, so recovery accelerates as the player recovers **[SRC]**.
- At game start `P = 160`, `D = 0` (see §10), giving `HEARTR = 46` jiffies.

## 10. Startup state relevant to timing

**[SRC]** `ONCE.ASM GAME10`, `COMDAT.ASM RAMDAT`:

- `PROW/PCOL = $10/$0B` (row 16, column 11) on level 0.
- `RAMDAT` presets `PPOW = $17A0`; `GAME10` then executes `CLR PPOW`, which
  clears only the **high** byte of the two-byte field, leaving `PPOW = $00A0 =
  160`. This is the game-mode starting power, and it is easy to misread as zero.
- `POBJWT = 30+5 = 35`, equal to the initial bag (wooden sword 25 + pine torch
  10).
- `PDIR` has no explicit initialisation; RAM is cleared beforehand, so north (0)
  **[INF]**.

## 11. Level transitions

**[SRC]** `NEWLVL.ASM`, `DGNGEN.ASM`, `COMCRE.ASM`:

- `NEWLVL` resets creature control blocks and their TCBs, rebuilds the maze from
  the fixed `LVLTAB` seed for the level, repopulates creatures from `CMTTAB`, and
  distributes creature-owned objects.
- `DGNGEN` seeds the RNG from `LVLTAB + LEVEL` and **only after the maze and all
  115 doors are complete** does `DGEN90` draw `SECOND` further random numbers.
  The five maps are therefore fixed; the *subsequent* RNG stream is not.
- `DGEN90` is `LDB SECOND / loop { RANDOM ; DEC B ; BNE }`. With `SECOND = 0` the
  loop runs **256** times, not zero **[SRC]** — a boundary a modern rewrite
  loses if it is written as `for (i = 0; i < second; ++i)`.
- Whether and when a `CREGEN` matrix increment becomes a visible creature — in
  particular on re-entering a level — is **[OPEN]**; `CREGEN` only increments the
  level matrix, and the creation path in `COMCRE` needs a dedicated trace.

## 12. Port obligations

1. `advanceJiffies(n)` must execute **n discrete boundaries**; a large delta may
   never skip intervening interrupt work.
2. Keystrokes must be timestamped individually. Timestamping completed commands
   loses queue saturation, the 32-byte overrun, and mid-burst dispatch.
3. Keep ordered lists for the countdown queues and the ready queue; FIFO append,
   traversal-order expiry.
4. Model `SYNC` as "this task owns the rest of the current jiffy"; do not fold a
   command's state change and its frame presentation into one instant.
5. Sound and animation must be able to block the foreground, because the original
   lets them.

## 13. Deviations in the reference slice

| ID | Deviation | Why |
|---|---|---|
| D-1 | The slice models per-jiffy foreground passes rather than the endless `SCHED` lap | A faithful endless loop would re-enter any task returning `Q.SCD` without bound; no in-scope task does that. Revisit before implementing creatures. |
| D-2 | Tasks made ready during a pass run on the following pass | Keeps the pass deterministic and terminating. The original would pick them up on its next lap, usually within the same jiffy. **[OPEN]**: measurable only against a ROM trace. |
| D-3 | `HSLOW` clamps a computed countdown of 0 to 1 | A 0 countdown would wrap to 255 in the original; the clamp avoids silently modelling a 255-jiffy delay. Only reachable at `HEARTR == 0`, i.e. near faint. |
| D-4 | Animation and sound cost no simulated time | Durations are unknown from the listing (§5.1). |
| D-5 | The trace samples the clock counters when an event is emitted | Interrupt-phase events can therefore print a pre-bump counter value. |
