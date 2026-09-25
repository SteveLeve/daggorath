# ROM equivalence — Phase 1 measurement

This is the only record of the Phase 1 attempt to assemble the pinned listing
and diff it against a retail cartridge. Other documents point here. They do not
restate the obstacle.

**Result: the pinned listing assembles. It has not been diffed against a retail ROM.** No claim in this repository is ROM-verified.

## What was measured

On 2026-09-25, `lwasm` from LWTOOLS 4.25 (source tarball
`https://www.lwtools.ca/releases/lwtools/lwtools-4.25.tar.gz`, built locally,
not installed system-wide) assembled `third_party/dod-asm` at
`a94326f00ebb16a106b540c58bc2ccf5f7b66dac` through `tools/rom/assemble.sh`.

| Image | Bytes | SHA-256 |
|---|---|---|
| Assembled cartridge (`--format=raw`) | 8192 | `35e6a77354dcf1a3048f276824b7a0f9f759115fdd40603664cebfb3a7da6571` |
| Retail ROM | none held | not computed |

The listing file produced 11701 lines. The assembled image is the size of an
8 KiB cartridge. That size is not a comparison with a retail image. No retail
bytes were read, so there is no differing-byte list and no claim here changes
from source-proven to ROM-observed.

`tools/rom/capture.lua` was run with `DOD_SELFTEST=1` under Lua 5.4.7. That
checked the watchlist parser, the symbol table, and the key table
(`selftest ok symbols=36 watches=36 script_keys=14` against
`t1-move-turn-look.script`). A missing symbol exits 1. The frame loop was not
entered. MAME and XRoar are not installed, and no CoCo ROM set was used. The
ioport tags in `capture.lua` are still unverified.

## What is still absent

| Requirement | Result |
|---|---|
| CoCo emulator | Not installed. No frame capture |
| Retail ROM | No image on the machine. None was fetched. No filename, hash, or rights basis |

## What this means for `reconciliation.md`

Every behavioural claim that Phase 0b and Phase 1 mark **source-proven** stays
source-proven against the reconstructed listing, including its 2022
`lwasm` edits. None of those claims is promoted to **ROM-observed**. The
deviations D-1 through D-5 in `clock-and-scheduler.md` §13 are unchanged by
capture, because there was no capture. Animation and sound durations stay open
for the same reason.

A later run that obtains a retail image records, in this file only:

1. The MAME or XRoar version and machine configuration.
2. The retail image's filename, byte length, SHA-256, and the rights basis for
   holding it. That row is added to `ledger.md` §1 **before** the image is used
   as evidence.
3. Every byte that differs from the assembled image above, and what the
   difference does to a specific claim.

Until that diff exists, source-derived traces and fixture hashes are not ROM
validation. The assembled hash above is a hash of this listing build, not of a
cartridge from 1983.
