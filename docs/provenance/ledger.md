# Provenance ledger — Phase 0b

Pinned 2026-09-24. Every artifact in this pack traces to a row below. Hashes are
SHA-256 unless stated.

## 1. Environment and tool versions

| Tool | Version | Used for |
|---|---|---|
| Python | 3.11.15 | `tools/extract_fixtures.py`, `tools/gen_lexicon_header.py` |
| g++ | 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1) | reference slice |
| CMake | 3.28.3 | reference slice build |
| Platform | Linux 6.18.44 x86_64 | build and test host |
| `lwasm` | LWTOOLS 4.25, built 2026-09-25 from the upstream tarball. Not installed on `PATH` | assembled the pinned listing. Byte comparison with catalog 26-3093 is in [`rom-diff.md`](rom-diff.md) |
| Emulator | MAME 0.264, Ubuntu package `mame 0.264+dfsg.1-1`. Driver `coco2b` (“Color Computer 2B”, clone of `coco`, same `trs/coco12.cpp` and keyboard matrix): MC6809E at 894886 Hz, 64K RAM (driver default), Color BASIC 1.3 and Extended Color BASIC 1.1 (no BIOS options), cartridge passed as `-cart`. Chosen 2026-09-25 to match the owner's CoCo 2. The first invocation used `coco` (default BIOS `b12e11`, Color BASIC 1.2) and is superseded. XRoar is not in the Ubuntu noble archive that was queried | First invoked 2026-09-25 as `coco` with `-video none -sound none -skip_gameinfo -seconds_to_run 1` and an empty ROM path. Exited before a frame: `bas12.rom` and `extbas11.rom` were missing. No capture was produced. See [`rom-diff.md`](rom-diff.md) |
| CoCo firmware | `bas13.rom` (Color BASIC 1.3) and `extbas11.rom` (Extended Color BASIC 1.1), 8192 bytes each. MAME 0.264 `coco2b` expects SHA-1 `28b92bebe35fa4f026a084416d6ea3b1552b63d3` and `ad927fb4f30746d820cb8b860ebb585e7f095dea`. Files used, both matching those SHA-1s: `bas13.rom` (archive filename `coco.rom`, from the archive's `coco2b.zip`) SHA-256 `5375b1caef9df815b781d9cd6df81ecdaddd2642b06a01adac521e40f1329751`; `extbas11.rom` SHA-256 `a18f270d3495d82b9f515de049f46253a623a2d9b1bb226c9ad338508344c5ee` | Rights basis: the owner holds a physical TRS-80 Color Computer 2 whose startup banner reports Color BASIC 1.3, a “B” model with the later video chip that `coco2b` emulates (owner report, 2026-09-25). Same basis as the cartridge row: the bytes are the Color Computer Archive images, not a dump of that machine. Microsoft/Tandy copyright; no redistribution grant. Held outside the repository in `DOD_FIRMWARE`, never committed. Used only to boot the emulator for captures |
| Retail ROM | `Dungeons of Daggorath (1982) (26-3093) (Tandy).ccc`, 8192 bytes, SHA-256 `35e6a77354dcf1a3048f276824b7a0f9f759115fdd40603664cebfb3a7da6571` | Owner states they hold a physical Tandy cartridge. Bytes are the Color Computer Archive file for catalog 26-3093, kept in gitignored `captures/`, not a dump of that cartridge and not committed. Byte-identical to the LWTOOLS 4.25 image. See [`rom-diff.md`](rom-diff.md). Not an emulator capture |

## 2. Primary evidence — reconstructed assembly listing

Repository: `https://github.com/MichaelSpencerJr/DungeonsOfDaggorath`
Pinned commit: `a94326f00ebb16a106b540c58bc2ccf5f7b66dac`
(merge of PR #24, dated 2022-01-30).

Kind: **primary implementation evidence with editorial reconstruction.** The tree
carries a 1983 Unified Technologies notice in `DAGGORATH.ASM`, has been adapted
for `lwasm`, and contains 2022 annotations and rebuilt macros (`missing-macros.asm`,
and inline `2022 n.b.` notes where indexed addressing was rewritten). It is
**not** a byte-verified retail ROM.

Rights: the repository contains `grant_of_license.png`, a handwritten grant naming
**Michael Spencer** and permitting copying/replicating/emulating the contained
source for the purpose of replicating the game. That grant is to a named person
and does not by itself relicense the tree to downstream recipients. Target use in
this project: **behavioural study and data extraction** (see §4 for the
distinction that matters).

Files read for this phase, with hashes as fetched:

| File | SHA-256 | Used for |
|---|---|---|
| `RANDOM.ASM` | `18827f5550a5d337ae2e4c8ba36af04908c9bc796967ba7c3457caa6bfc8131b` | RNG |
| `DGNGEN.ASM` | `27f9ea8873cafd7cc824231aa2860b2b709a899a29b3e5f59c472641c1e45922` | maze generation, `LVLTAB` |
| `CRETUR.ASM` | `4c3ad917ee9398840a1f1cc877d5da3fca0614ba35b27f4a98fbe50db06bba80` | `STEP`, `STEPOK`, `STPTAB` |
| `TOKEN.ASM` | `7c6f179bc94011f0a1e4495f3fb3ced7dbaae0597c0a6bf61be6de6f8b12ca83` | lexicon |
| `EXPAND.ASM` | `92a5fa9665b8903aca31999a2088823df0fed565ce7a8d409f24d4e7be9249a4` | 5-bit string decoding |
| `PARSER.ASM` | `14791ad8433855635281051dd0fa2cc7ae887f033877b14a955eaeaf1c5f7c0e` | parser semantics |
| `DTABAS.ASM` | `2f63459b15ae85854b03c30a937880995251af1b5172d4131d1ca17e5bfa3b2a` | creature/object tables, dispatch |
| `COMDAT.ASM` | `90efe81bbfc084e2792c949625c33b628b6721ae5c2cbc16163d629ee0f82bed` | `CMTTAB`, `TCBDAT`, `RAMDAT`, `GAMDAT` |
| `COMMON.ASM` | `9a7f8457167fb711d5309c8412810389d0922930cf5b367ab1a6e5e5faa1ab82` | `CLOCK`, `QUESCN`, `SCHED`, queues, keyboard |
| `HUMAN.ASM` | `a3b4e70211c3ebe344ab1c31a12833690837ebce33a0fdf038b65fe03ca5dbe2` | `PLAYER`, `HUMAN` |
| `PTURN.ASM` | `0d57b0848187d5be02398856b28099560cf4d47f326186b433cca4cc250ff71e` | `PTURN`, `PMOVE`, `PSTEP` |
| `COMPLR.ASM` | `8cc68e03c40720ee66425fdc2badd80add155f767e83245fd48fa990a7d54dd5` | `BURNER`, `LUKNEW`, `HSLOW` |
| `HUPDAT.ASM` | `2569853b32a0df334d590519df84ce49595b9c903f1ccfd12b2f4748e1f0a2e4` | heart rate, faint, death |
| `COMCRE.ASM` | `352fec3bb8767449c52e376dfaa2171fa05556d7e6303c3f5251660cf3d0bf49` | `VFTTAB`, `CREGEN` |
| `ONCE.ASM` | `d4420634a38b087965b409b511d8b4ebd8b122340aa2189f9defb1444fd6c19e` | `SYSTCB`, `GAME10`, `IRQSYN` |
| `CD.ASM` | `c991fe0ba18b9ab42ac29304bd9b8f643ecad874743f9cf9ee64ff64cb88e20a` | RAM map, constants, TCB layout |
| `MISC.ASM` | `4940b3cd058d8640424fb35a8fac2202ca6c5b3ff7640342d4a7c3309ef838cd` | `NEGRAM`, `WAITX` |
| `grant_of_license.png` | `423a4c1444977be55212d81ecd21ed7fde6dfce4880d16cc163f6593e5087162` | rights evidence |

## 3. Other sources

| Source | Kind | Rights position | Used in Phase 0b |
|---|---|---|---|
| Morgan preservation grant (`iloveglory.freehostia.com/daggorath/license.html`, transcribed in the 3DS port) | Licensing statement for *the game*, via later mirrors | Nonexclusive worldwide licence to develop/produce/duplicate/emulate/distribute the game, conditioned on preserving its original unaltered form; asserts a belief about reversion from Radio Shack rather than supplying the underlying contract | **Not re-fetched this phase.** Carried forward from Phase 0; re-verify and archive a dated copy before publication |
| Tandy 1983 manual (archive.org OCR) | Contemporary documentation | Distinct ©1983 Tandy notice; paraphrase only, do not bundle | Not used for any fixture value this phase |
| Hunerlach Linux/SDL port | Secondary adaptation | No clear standalone grant for the port's C++/WAV code | **Not consulted, not fetched** |
| cognitivegears SDL2/WebAssembly core | Secondary adaptation | Top-level code licence undetermined | **Not consulted, not fetched** |
| 3DS port | Secondary adaptation | GPL-3.0 | **Not consulted, not fetched** |

No third-party port was read or cloned during Phase 0b. Everything in this pack
derives from the assembly listing alone, which keeps the copyleft and
undetermined-licence ports out of the provenance chain entirely.

## 4. Derived artifacts produced in this phase

| Artifact | Derivation | Rights character |
|---|---|---|
| `fixtures/rng.json`, `rng-vectors.txt` | computed by an independent transliteration of `RANDOX` | new work embedding **behaviour**, not source bytes |
| `fixtures/mazes.json`, `maze-level-0..4.bin` | computed by an independent transliteration of `DGNGEN` from the `LVLTAB` seeds | the seed bytes (7 bytes) are copied data; the 5 KiB of maze output is computed |
| `fixtures/tokens.json`, `parser-prefixes.json` | decoded from `TOKEN.ASM`'s packed strings | **copied data**: the player-facing lexicon is verbatim original content |
| `fixtures/creatures.json`, `objects.json`, `vertical-features.json` | parsed from `DTABAS.ASM`, `COMDAT.ASM`, `COMCRE.ASM` macro arguments and `FCB` lists | **copied data**: original tuning tables |
| `fixtures/clock.json`, `initial-state.json` | constants read from `COMMON.ASM`, `CD.ASM`, `ONCE.ASM`, `COMDAT.ASM` | small constants; low risk but still copied |
| `reference/**` | written for this project from the behavioural reading | new project code |
| `reference/include/daggorath/lexicon_tables.hpp` | **generated** from `fixtures/tokens.json` | carries copied data into the build; treat as a licensed artifact |
| `traces/*.trace` | output of the reference slice | new work |

**Open rights question, unchanged from Phase 0 and now sharper:** the fixtures in
the "copied data" rows are original tuning and lexicon data, not behaviour. They
are the artifacts that would need the Morgan grant (or specific permission) to
ship. Recommendation: keep them in a single directory with their own notice, keep
the generated `lexicon_tables.hpp` clearly marked as generated, and put both in
front of counsel before any public distribution. Nothing in this pack should be
relicensed under the project's eventual terms without that review.

## 5. ROM and emulator

Phase 0b found no ROM, no emulator, and did not run `lwasm`. Phase 1 recorded
the later measurement in [`rom-diff.md`](rom-diff.md). The assembled listing
matches the catalog 26-3093 image byte for byte. MAME 0.264 was then invoked
on that image and exited before a frame, because Color BASIC 1.2 and Extended
Color BASIC 1.1 were not legally obtained. No frame was captured, so
behavioural claims are not ROM-observed. Maze hashes remain hashes of this
project's serialization, cross-checked between the Python extractor and the
C++ core.

Phase 2 added no fixture and no ROM. `docs/archaeology/phase-2/traces/idle-10min.trace`
is `dcli` output from the core, not a capture. Creature delays were already in
`fixtures/creatures.json` (`DTABAS.ASM` `CREXXX`).
