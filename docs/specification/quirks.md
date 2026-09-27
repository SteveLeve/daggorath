# Quirks ledger

This is the canonical list of Original Mode quirks. Phase reports keep their
dated tables; new rows are added here. Implementation deviations that are not
original behaviour live in [`clock-and-scheduler.md`](clock-and-scheduler.md) §13.

Classification follows `docs/project-instructions.md`: intentional behaviour,
implementation artifact, comment error, or a discrepancy between two sources.
"Preserve" means Original Mode reproduces it.

## Phase 0b

| Observation | Classification | Original Mode |
|---|---|---|
| `PLAYER` drains the whole keyboard buffer per turn | intentional | preserve; timestamp keystrokes individually |
| 32-byte keyboard buffer with no overflow check | accepted limitation | preserve |
| `LINBUF` full dispatches the line without Return | likely intentional | preserve; regression-tested |
| `CLR PPOW` clears one byte of a two-byte field | deliberate | preserve; starting power is 160 |
| `LUKNEW` comment says "twice per second", code says 3 tenths | comment error | follow the code (0.3 s) |
| `WAITX` comment says 1.5 s, code is 81 jiffies (1.35 s) | comment error | follow the code |
| `MAKDOR` retries until an edge's 2-bit field is `00` | source behaviour | preserve; the retries consume RNG draws |
| `DGEN90` spins 256 times at `SECOND = 0` | source behaviour | preserve |
| Movement legality ignores edge bits | source behaviour, self-consistent | preserve; do not add a wall test |
| `ADJTAB` word for the `HOTH` object is `RIME` | naming difference | ship `RIME` |
| `DIRTAB` entry is `BACK`, so `BACKWARD` is rejected | source/manual discrepancy, resolved for `BACK` | ship `BACK` |

## Phase 1

| Observation | Classification | Original Mode |
|---|---|---|
| `CREGEN` increments a matrix count and does not call `CBIRTH` | source behaviour | preserve; the creature appears on the next `NEWLVL` |
| The opening scheduler lap runs `CREGEN` because tasks start in `Q.SCD` | source behaviour | preserve |
| Cell (0, 0) cannot be a birth cell | implementation artifact of marking the block in use before storing its position | preserve |
| Shields, swords, and torches carry generic parameters until `REVEAL` | source behaviour | preserve |
| `NEWLVL` object attachment spins until it finds a live creature (`NLVL42`) | source behaviour | preserve. With no live creature the original never returns, and this core loops the same way. Every shipped level births at least one creature before attachment, so the loop terminates for those rows |
| `CMOVE` is not queued | retired 2026-09-25 | was D-6; `CBIRTH` queues `CMOVE` |

## Phase 2

| Observation | Classification | Original Mode |
|---|---|---|
| Frozen is tested before death, so a frozen dead creature stays queued at the movement delay | source behaviour | preserve |
| A dead creature returns with `B = 0`, and `SCHED` parks it on queue 0 | source behaviour | preserve by dropping the task; queue 0 is not scanned |
| Scorpions and both wizard types do not pick objects up | source behaviour | preserve |
| `MOVTAB` tries a side before forward when the random byte's low two bits are 0 | source behaviour | preserve; 64 of 256 |
| `CMOVE` stops at `JSR ATTACK` | retired 2026-09-25 | was D-7; the attack branch now calls `ATTACK` |
| The starting wooden sword and pine torch get `P.OCLVL` = `$0B` (`GAME10` entry only; the demo path never runs `LDD #$100B`): `GAME30` (`ONCE.ASM:305-308`) calls `OBIRTH` without loading `B`, which still holds the low byte of `LDD #$100B` (`ONCE.ASM:286`); the `SWI` dispatcher reloads and restores it (`COMSWI.ASM:33,37`) and `OBIRTH.ASM:22` stores it | source behaviour | preserve (run 27). No level lookup sees it: `OFIND` and `MAPPER` skip owned objects, `NLVL40` skips them (`NEWLVL.ASM:61-62`, `BPL`), and `PDROP` rewrites the level (`PGET.ASM:76-77`). It is still in the OCB table, so a saved RAM image carries it |
| `EXAMINE`: a `PRTOBJ` tab (`PEXAM.ASM:154-157`) from the second half of row 18 leaves the cursor at 608; the next name's first character is drawn at column 0 of the status line (`COMDAT.ASM:93`, 98), then `TXTSER` scrolls the 19-line page, leaving it there | bug (original) | not reproduced: the examine pad drops that character (`src/presentation/examine.cpp`); visibility before the status redraw is [INF]. Run 25 |
