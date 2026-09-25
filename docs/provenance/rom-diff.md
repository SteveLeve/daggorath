# ROM equivalence — Phase 1 measurement

This is the only record of the comparison between the pinned listing and a
retail cartridge image. Other documents point here.

**Result: the assembled listing and the Tandy 26-3093 cartridge image are byte-identical.** No emulator frame has been captured, so timing claims are not ROM-observed.

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
`t1-move-turn-look.script`). The frame loop was not entered. MAME and XRoar
are not installed, and no Color Computer firmware ROM was used. Deviations
D-1 through D-5, and the animation and sound durations, are unchanged.
