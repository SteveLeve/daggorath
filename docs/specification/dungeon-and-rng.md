# Dungeon and RNG

Living specification for the generator, the five maps, movement legality, and
the vertical-feature table. Every rule is labelled. Nothing here is
ROM-observed; the measurement that would change that is recorded in
[`../provenance/rom-diff.md`](../provenance/rom-diff.md).

Original Mode uses the fixed `LVLTAB` seeds. It does not take a player seed and
it does not build a new map.

## 1. RNG

**[SRC]** `RANDOM.ASM RANDOX`. The state is three bytes, `SEED` through
`SEED+2`. Each draw runs eight rounds. A round masks `SEED+2` with `$E1`,
counts the set bits of that masked byte, and rotates the count's low bit into
a 24-bit rotate-left of the three seed bytes. The value returned is `SEED[0]`
after the eight rounds.

Fixtures: `fixtures/rng.json`, `fixtures/rng-vectors.txt`. The C++ core and the
Python extractor implement this independently and agree on the vectors.

## 2. Level seeds

**[SRC]** `DGNGEN.ASM LVLTAB`. The table is seven bytes
`73 C7 5D 97 F3 13 87`. Level *n* uses bytes *n*, *n+1*, *n+2*. The windows
overlap. The commented-out `$25` debug byte is not in the table.

## 3. Maze construction

**[SRC]** `DGNGEN.ASM DGNGEN`, `MAKDOR`, `DGEN90`.

1. Fill all 1024 cells with `$FF`.
2. Seed the RNG from the level's `LVLTAB` window.
3. Carve 500 cells. `RNDCEL` draws the **column first**, then the row, each
   masked with 31. A run length is `(random & 7) + 1` in a direction
   `random & 3`. A step is rejected when it would leave the 32×32 border or
   when any of the four 3-byte corner sums over the 3×3 neighbourhood is zero.
   At that stage cells are only `$00` or `$FF`, so a zero sum means the three
   cells are already carved: the test refuses a 2×2 open block.
4. Raise a wall bit on a carved cell toward a neighbour that is still `$FF`.
5. Place 70 regular doors, then 45 secret doors. Each attempt retries until it
   finds an edge whose 2-bit field is still `00` (a passage). An edge therefore
   never holds both a regular and a secret door, and the retries consume RNG
   draws.

**[SRC]** Only after the maze and all 115 doors are complete does `DGEN90` spin
the RNG. The loop is `LDB SECOND / DEC B / BNE`, so `SECOND = 0` spins **256**
times, not zero. The 1024 maze bytes do not depend on `SECOND`. Creature
placement and everything else drawn afterwards do.

Generated at `SECOND` = 0, 1, 7, 30, 59, and on a repeat entry at the same
`SECOND`, all five levels produce byte-identical mazes
(`fixtures/mazes.json`, `entry_time_invariance`). **Unverified against ROM.**

Serialization, which the hashes depend on: 1024 bytes, row-major, index
`row*32 + col`, one byte per cell, bit pairs low-to-high North/East/South/West,
`00` passage, `01` regular door, `10` secret door, `11` wall.

## 4. Movement legality

**[SRC]** `CRETUR.ASM STEP`, `STEPOK`. A step is legal when the destination is
inside the 32×32 border and the destination cell is not `$FF`. There is no
per-edge wall test. Doors do not block movement. Phase II only raises a wall
bit toward an `$FF` neighbour, and `MAKDOR` only places a door on an edge that
is already a passage, so the missing edge test is consistent with the
generator. All 500 carved cells on level 0 are reachable from the start cell
under this rule.

`STEP` adds the direction delta in 8 bits. `BORDER` then requires that masking
with 31 leaves the coordinate unchanged.

## 5. Vertical features

**[SRC]** `COMCRE.ASM VFTTAB`, `NEWLVL.ASM NLVL10`/`NLVL12`, `CD.ASM`
`VF.HUP`/`VF.LUP`/`VF.HDN`/`VF.LDN`.

`VFTTAB` records are `kind, row, col`, terminated by `$80`. Kind 1 is a ladder
and kind 0 is a hole in the table's own comments. `NEWLVL` walks the table
once per level number: it records `VFTPTR`, scans forward to a negative byte,
then decrements the level counter and repeats while that counter stays
non-negative. The pointer it leaves is the start of that level's search.

`VFIND` reads upward from `VFTPTR` until a negative byte, and if that fails
continues into the next group and adds 2 to the kind. The `CD.ASM` codes are
hole-up 0, ladder-up 1, hole-down 2, ladder-down 3, which is what that add-2
produces. The byte indexes `NEWLVL` computes are in
`fixtures/vertical-features.json` (`newlvl_vftptr_byte_index`):

| Level | `VFTPTR` byte index | What the search sees |
|---|---|---|
| 0 | 0 | the leading `$80`, so "up" is empty and "down" is the following group |
| 1 | 1 | that first feature group upward, and the next group downward |
| 2 | 14 | the second feature group upward; the following byte is `$80`, so "down" is empty |
| 3 | 27 | a `$80`, so "up" is empty and "down" is the last feature group |
| 4 | 28 | the last feature group upward; the table then ends |

The empty link between levels 2 and 3 is the pair of consecutive `$80` bytes.
It is in the table, not a grouping error. Climb behaviour that consumes these
codes is not specified here.

## 6. What stays unresolved

- Whether the listing's maze bytes match a retail ROM. See `rom-diff.md`.
- Whether `SOUNDS:SNOISE` consumes `SEED`. `SOUNDS.ASM` was not traced for this
  document.
