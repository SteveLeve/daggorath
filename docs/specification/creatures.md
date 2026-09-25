# Creatures

What Phase 1 establishes about level population and regeneration. Creature
movement, attacks, damage, and death are named where the listing places them
and are not specified as implemented behaviour. The population rules below
are source-proven. The level-0 opening lap at `SECOND` = 6 (type 5, 24 live,
25 on the next `NEWLVL`) is ROM-observed in
[`../archaeology/phase-1/reconciliation.md`](../archaeology/phase-1/reconciliation.md) §1.
`population-entry.txt` remains the source-derived `SECOND` = 1 comparison.

## 1. Definition block

**[SRC]** `DTABAS.ASM CREXXX`, `CDBTAB`, `CD.ASM`. Twelve types, index 0
through 11: spider, viper, two giants, blob, two knights, scorpion, wraith,
balrog, wizard image, wizard. Each definition is power (16-bit), magic
offense, magic defense, physical offense, physical defense, movement delay,
attack delay. The delays are tenths of a second because `CBIRTH` queues
`CMOVE` on `Q.TEN`.

The numbers are in `fixtures/creatures.json`.

## 2. The matrix is the budget

**[SRC]** `COMDAT.ASM CMTTAB`, `CD.ASM CMXLND`, `ONCE.ASM` RAM initialisation.

`CMTTAB` is 5 rows by 12 counts. Those 60 bytes are the initial contents of
`CMXLND`. They ride in the `RAMDAT` image that `ONCE` copies to `TXTEXA` for
`(TX.LEN*3)+(CTYPES*5)` bytes, and `CMXLND` sits immediately after the three
text blocks. `NEWLVL` does not copy `CMTTAB` again. It reads `CMXLND` through
`CMXPTR`, which it sets to `CMXLND + LEVEL*12`.

Counts for the five levels are 24, 24, 23, 24, and 31.

## 3. Birth

**[SRC]** `NEWLVL.ASM NLVL30`, `COMCRE.ASM CBIRTH`, `FNDCEL`, `CFIND`.

`NEWLVL` zeros every creature control block, rebuilds the system tasks, builds
the maze, then walks creature types from 11 down to 0. For each type it calls
`CBIRTH` once per matrix count.

`CBIRTH`:

1. Takes the first control block whose `P.CCUSE` is zero and decrements that
   byte. A zeroed byte becomes `$FF`. The block is in use before it has a
   position, and its row and column are still 0.
2. Copies the 8-byte definition onto the block and stores the type.
3. `FNDCEL` draws `RNDCEL` (column, then row) until the cell is not `$FF`.
   `CFIND` rejects the cell when any in-use block already has that row and
   column. The block being filled is in use at (0, 0), so **(0, 0) is never
   chosen**.
4. Stores the position. Direction and damage stay 0. The movement delay is
   the definition's movement delay.

`CBIRTH` then allocates a task control block for `CMOVE` on `Q.TEN` with that
delay. **The core does not queue that task** (deviation D-6). Movement and
attacks are not implemented. The control-block fields above are.

Fixtures: `fixtures/population-entry.txt` and `fixtures/population.json`, for
every level at `SECOND = 1`, and for level 0 at `SECOND` = 0, 1, 7, 30, 59.
Positions change with `SECOND`. Counts do not. No live creature is at (0, 0).

## 4. Objects hung on creatures

**[SRC]** `ONCE.ASM CINI40`, `OBIRTH.ASM`, `NEWLVL.ASM NLVL40`.

Before the first `NEWLVL`, `ONCE` creates one object control block per
`OMXTAB` entry. The high nibble is the starting level and the low nibble is
the count. Each copy is born at the current level, then the level increments,
and when the level would pass 5 it returns to the starting level. `DEC` of the
zeroed ownership byte stores `$FF`, which `NEWLVL` treats as creature-owned
(`TST` / `BPL` skips a non-negative owner).

`OBIRTH` copies the definition, then for a shield, sword, or torch copies the
leather, wooden, or pine definition over it and restores the original reveal
requirement (`GENVAL`). Unrevealed objects therefore carry the generic
offense bytes and, when the generic type has an `XXXTAB` row, the generic
special parameters. `REVEAL` later calls `OCBFIL` with the real type. Reveal
itself is not implemented.

`NEWLVL` walks objects on the current level whose owner is negative and, for
each, the next live creature, wrapping at the end of the 32 slots. `NLVL42`
does not give up when every block is unused: it keeps scanning. The core does
the same. Shipped populations birth at least one creature before this walk, so
the scan ends. The object is prepended to that creature's list. Objects on other levels are left alone.
The player's wooden sword and pine torch are created in `GAME30` **after**
`NEWLVL` returns, with ownership incremented to 1, so they are not in the
first attachment. A later `NEWLVL` sees them and skips them because the owner
is positive.

## 5. Regeneration

**[SRC]** `COMCRE.ASM CREGEN`, `PATTK.ASM PATT40` (death, not implemented).

`CREGEN` sums the twelve counts on the current row in an 8-bit accumulator.
If the sum is below 32, it draws one random byte, takes `(byte & 7) + 2`, and
increments that type. Types 0, 1, 10, and 11 are never selected. If the sum
is 32 or more, the matrix is unchanged. The task then returns
`SCHED$ 5,Q.MIN`.

`CREGEN` does not call `CBIRTH`. The incremented count becomes a live creature
the next time `NEWLVL` runs for that level, because birth reads the matrix
and the matrix is not reset. While the player stays on the level, the extra
creature does not exist.

System tasks are created in `Q.SCD` with countdown 0 (`ONCE.ASM SYSTCB`), so
the first `CREGEN` runs on the opening scheduler lap, not after five minutes.
On level 0 at `SECOND = 1` that draw increments type 9 (balrog) from 0 to 1.
Live creatures stay 24. Entering the level again at the same `SECOND` births
25. That row is the source-derived comparison in `population-entry.txt`
(`cregen 24 24 25 9 25`). Original Mode level 0 is the ROM entry, `SECOND` = 6
after 377 build interrupts: the opening lap increments type 5, the live count
stays 24, and the next `NEWLVL` births 25. The listing schedules the later
runs five minute-queue scans apart (`SCHED$ 5,Q.MIN`). The captured window
did not show that reschedule; why is unresolved in reconciliation §5.

Killing a creature, which is not implemented, is the path that decrements the
matrix (`PATT40`). The core therefore never lowers a count.

`CREGEN` only edits the current level's row. Time spent on another level does
not regenerate this one.

## 6. Not in this specification

`CMOVE`'s priorities, attack resolution, frozen and dead creatures, and the
wizard-death sequences are unread for implementation purposes. Queuing `CMOVE`
is recorded as D-6 so those routines are not smuggled in as inert tasks that
would change the foreground order.
