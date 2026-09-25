# ROM equivalence — Phase 1 measurement

This is the only record of the comparison between the pinned listing and a
retail cartridge image. Other documents point here.

**Result: the assembled listing and the Tandy 26-3093 cartridge image are byte-identical.** No emulator frame has been captured. A frame capture was attempted and stopped because Color BASIC and Extended BASIC could not be legally obtained. Timing claims are not ROM-observed.

## Rights basis

The project owner states that they own a physical copy of the Tandy cartridge.
The bytes compared here are the Color Computer Archive file for catalog
**26-3093**, not a dump taken from that physical cartridge in this session.
The file is local, under `captures/`, and is gitignored. It is not in the
repository.

| Field | Value |
|---|---|
| Filename | `Dungeons of Daggorath (1982) (26-3093) (Tandy).ccc` |
| Archive path | `https://colorcomputerarchive.com/repo/Cartridges/Dungeons%20of%20Daggorath%20(1982)%20(26-3093)%20(Tandy).ccc` |
| Bytes | 8192 |
| SHA-256 | `35e6a77354dcf1a3048f276824b7a0f9f759115fdd40603664cebfb3a7da6571` |

The Shield Fix, disk patch, cassette, and disk images on that archive were not
used.

## Assembled listing

On 2026-09-25, `lwasm` from LWTOOLS 4.25 (source tarball
`https://www.lwtools.ca/releases/lwtools/lwtools-4.25.tar.gz`) assembled
`third_party/dod-asm` at `a94326f00ebb16a106b540c58bc2ccf5f7b66dac` through
`tools/rom/assemble.sh` (`--6809 --format=raw`).

| Image | Bytes | SHA-256 |
|---|---|---|
| Assembled cartridge | 8192 | `35e6a77354dcf1a3048f276824b7a0f9f759115fdd40603664cebfb3a7da6571` |
| Archive `.ccc` | 8192 | `35e6a77354dcf1a3048f276824b7a0f9f759115fdd40603664cebfb3a7da6571` |

`cmp` of the two files exited 0. Differing bytes: **0**. The `.ccc` has no
header; all 8192 bytes are the cartridge image.

The 2022 assembler-compatibility edits in the pinned listing assemble to the
same bytes as this catalog 26-3093 image. Source-proven claims read from that
listing are claims about the code in this image. They are not yet observations
of a running machine.

## What a capture has not done

`tools/rom/capture.lua` was run with `DOD_SELFTEST=1` under Lua 5.4.7
(`selftest ok symbols=36 watches=36 script_keys=14` against
`t1-move-turn-look.script`). The frame loop was not entered.

MAME 0.264, Ubuntu package `mame 0.264+dfsg.1-1`, was run on 2026-09-25.
In that version the CoCo 1/2 driver is `coco`; the later CoCo 2 is `coco2b`
(there is no system named `coco2`).
Its default BIOS is `b12e11`: Color BASIC 1.2 and Extended Color BASIC 1.1.
The launch was `mame coco -cart` of the catalog 26-3093 `.ccc`, with
`-video none -sound none -skip_gameinfo -seconds_to_run 1`, and an empty
ROM path. MAME exited before any frame:

```
bas12.rom NOT FOUND (tried in coco)
extbas11.rom NOT FOUND (tried in coco)
Fatal error: Required files are missing, the machine cannot be run.
```

| File | Role | SHA1 MAME requires |
|---|---|---|
| `bas12.rom` | Color BASIC 1.2, 8192 bytes | `0f14dc46c647510eb0b7bd3f53e33da07907d04f` |
| `extbas11.rom` | Extended Color BASIC 1.1, 8192 bytes | `ad927fb4f30746d820cb8b860ebb585e7f095dea` |

Those two images are not produced by assembling the game listing. None were
on this machine. They are Microsoft BASIC licensed to Tandy, and no
redistribution grant was found. They were not downloaded. The owner’s
physical cartridge is the game, catalog 26-3093, not these machine ROMs.
XRoar is not in the Ubuntu noble archive that was queried.

## Firmware decision, 2026-09-25

The owner holds a physical CoCo 2 whose startup banner reports Color BASIC
1.3. Captures therefore use MAME `coco2b` (Color BASIC 1.3, Extended Color
BASIC 1.1) rather than `coco` `b12e11`. The rights basis is recorded once,
in [`ledger.md`](ledger.md) §1. `tools/rom/run-capture.sh` refuses any
firmware whose SHA-1 is not:

| File | Role | SHA1 MAME `coco2b` requires |
|---|---|---|
| `bas13.rom` | Color BASIC 1.3, 8192 bytes | `28b92bebe35fa4f026a084416d6ea3b1552b63d3` |
| `extbas11.rom` | Extended Color BASIC 1.1, 8192 bytes | `ad927fb4f30746d820cb8b860ebb585e7f095dea` |

**Superseded 2026-09-25:** with the `coco2b` firmware in place, the five
Phase 0b scripts were captured. Results are in
[`../archaeology/phase-1/reconciliation.md`](../archaeology/phase-1/reconciliation.md) §1.
The block quote is the record of the state before that run.

> No frame was captured. No timing was measured or invented. Deviations D-1
> through D-5, and the animation and sound durations, are unchanged. The
> “not captured” rows in
> [`../archaeology/phase-1/reconciliation.md`](../archaeology/phase-1/reconciliation.md)
> §1 stand.
