# Combat and items

Combat is specified here. Item commands (`GET`, `USE`, `INCANT`, `CLIMB`, torches) are Phase 4.

Labels: **[SRC]** means the pinned listing at `a94326f`. No claim below is ROM-observed.

## Player attack (`PATTK`) — [SRC]

`PARHND` requires `LEFT` or `RIGHT`. Anything else is `???` and does not swing.

The hand's object supplies `P.OCMGO` and `P.OCPHO`. An empty hand uses `EMPHND` (`COMDAT.ASM`): class 4, magic offense 0, physical offense 5.

Exertion is `SCAL16(PPOW, (P.OCPHO + P.OCMGO) >> 3)` added to `PDAM` with 16-bit wrap. The shift is the 9-bit sum (`ADDA` carry, then `RORA` / `LSRA` / `LSRA`). The object's sound is emitted at full volume. Ring types `T.RN11` through `T.RN12` (token indices 19–21) decrement `P.OCXXX` and become `T.RN20` (22) when the counter lands on zero. Decrement of zero wraps to 255 and does not transform.

`CFIND` selects the creature in the player's cell. No creature ends the command at `HUPDAT`.

A ring (`K.RING`) skips `ATTACK` and the darkness gate. Otherwise `ATTACK` runs; N set is a miss. A miss, or a darkness failure, still calls `HUPDAT`.

Darkness: if `PTORCH` is zero or the torch type is `T.TOR5` (24), one `RANDOM` byte must have both low bits clear (1 in 4). Otherwise the swing misses.

A hit calls `DAMAGE`. `BHI` means the creature is still alive (`power > damage`, unsigned). Otherwise `PATT30`.

## `ATTACK` — [SRC]

Defender power minus defender damage, 16-bit, shifted left twice. That value is subtracted by attacker power until it borrows or the index falls from 15 to 0. Index minus 3: non-negative yields `index * 10`; negative yields `-(magnitude * 25)`. The RNG byte (zero-extended) is added. `SUBD #127` connects when N is clear, including a result of zero.

The Phase 0 note "VERIFY exact byte wrapping and signed flags" is closed for this routine: the comparison is the N flag of that 16-bit subtract, and `SCAL16` keeps the low 16 bits of `(value * radix) >> 7`. Fixtures cover the samples in `docs/archaeology/phase-3/fixtures/scal16-damage.json`.

## `DAMAGE` and `SCAL16` — [SRC]

Magical damage, then physical. Each channel is `SCAL16(SCAL16(attacker power, offense), defense)` added to defender damage with 16-bit wrap. `SCAL16` multiplies an unsigned 16-bit value by a radix-7 byte and shifts right 7.

## Kill (`PATT30`, `PATT40`) — [SRC]

Objects on the creature are unowned and placed on its cell. `CMXLND[type]` is decremented with byte wrap. The block's in-use flag is cleared. Player power gains an arithmetic `>> 3` of the creature's power. If that sum is negative, the high byte is forced to `0x7F` and the low byte is kept.

Creature type 10 emits `DEFER endgame 10` and does not run `ENDGAM` (Phase 5). Type 11 sets the freeze flag and emits `DEFER endgame 11`.

## Creature attack (`CMOVE`) — [SRC]

On the player's cell the creature plays a full-volume sound, sets shielding to `$8080`, and upgrades it when a held shield's two bytes are strictly smaller. `ATTACK` then `DAMAGE` on a hit. `HUPDAT` follows either way. The old `DEFER creature-attack` trace is retired (D-7).

## Objects — [SRC]

`PGET` / `PDROP` / `PSTOW` / `PPULL` (`PGET.ASM`). `GET` and `PULL` require an empty hand and either a generic name or an adjective plus a matching generic. `GET` marks the floor object owned and adds `OBJWGT[class]` (5, 1, 10, 25, 25, 10). `DROP` clears the hand, plants the object on the current cell and level, and subtracts that weight. `STOW` prepends the hand to `BAGPTR` without a weight change. `PULL` unlinks a bag object into the hand and clears `PTORCH` when that object is the lit torch. There is no bag-capacity test.

`REVEAL` (`PREVEA.ASM`) requires `P.OCREV * 25 <= PPOW`, then `OCBFIL` and a cleared reveal byte. `EXAMINE` selects the examine display mode.

`USE` (`PUSE.ASM`). A torch is stored in `PTORCH` and stowed. Flasks: `THEWS` adds 1000 power, `HALE` clears damage, `ABYE` adds `SCAL16(PPOW, 102)` to damage, then the flask becomes `EMPTY`. A revealed `VISION` scroll selects the mapper without features. A revealed `SEER` scroll selects the mapper with features. An unrevealed scroll does nothing.

`INCANT` (`PINCAN.ASM`) requires the full adjective. A held ring whose `P.OCXXX+1` equals that token becomes that type via `OCBFIL`, and the byte is cleared. The final ring emits `DEFER winner` (Phase 5 runs `WINNER`).

`BURNER` (`COMPLR.ASM`) runs once a minute. It decrements the torch timer. At 5 or below the torch type becomes `DEAD`. The regular and magic light bytes are lowered to the timer when the timer is smaller.

## Faint, recovery, death (`HUPDAT`) — [SRC]

Heart delay is the existing repeated-subtraction form. Not fainted and delay `<= 3` faints: the scheduler stops polling the keyboard, and `PLAYER` eats any character already buffered. Fainted and delay `> 4` recovers. Death is `PPOW < PDAM` (unsigned `BLO`), which is stricter than a creature's `BHI` kill. `DEATH` halts: this core stops the scheduler, matching `BRA *`.
