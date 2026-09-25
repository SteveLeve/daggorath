# ROM equivalence — Phase 1 measurement

This is the only record of the Phase 1 attempt to assemble the pinned listing
and diff it against a retail cartridge. Other documents point here. They do not
restate the obstacle.

**Result: not measured.** No claim in this repository is ROM-verified.

## What was available

| Requirement | Result on 2026-09-25 |
|---|---|
| Pinned listing `a94326f` | Present after `make sources`, under `third_party/dod-asm` (not committed) |
| `lwasm` | Not installed. `command -v lwasm` is empty. `apt-get` is present and `apt-cache search lwtools` returns no package |
| CoCo emulator (`mame`, `xroar`) | Not installed. No `coco` binary on `PATH` |
| Retail ROM | No image on the machine. None was fetched. Rights to hold one were not established, so none was added to the ledger as evidence |

`tools/rom/assemble.sh` exits 2 while `lwasm` is missing and does not invent a
cartridge hash. `tools/rom/capture.lua` was not executed.

## What this means for `reconciliation.md`

Every behavioural claim that Phase 0b and Phase 1 mark **source-proven** stays
source-proven against the reconstructed listing, including its 2022
`lwasm` edits. None of those claims is promoted to **ROM-observed**. The
deviations D-1 through D-5 in `clock-and-scheduler.md` §13 are unchanged by
capture, because there was no capture. Animation and sound durations stay open
for the same reason.

A later run that does obtain `lwasm` and a ROM records, in this file only:

1. `lwasm` version and the MAME or XRoar version and machine configuration.
2. The retail image's filename, byte length, SHA-256, and the rights basis for
   holding it. That row is added to `ledger.md` §1 **before** the image is used
   as evidence.
3. The assembled image's length and SHA-256.
4. Every differing byte, and what the difference does to a specific claim.

Until those four exist, source-derived traces and fixture hashes are not ROM
validation.
