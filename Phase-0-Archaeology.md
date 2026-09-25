# Dungeons of Daggorath — Phase 0 source archaeology

**Research snapshot:** 24 September 2026. **Status:** substantive architectural reconstruction; **not yet a complete executable behavioral specification**. No production code was written. “Primary source” below means the published reconstruction of a 1983 assembly listing, or the contemporary Tandy manual. It does not mean a byte-verified retail ROM. Source links are pinned to the inspected commits in [References](#references).

## 1. Executive summary

The game runs continuously while commands are typed. A 60 Hz interrupt maintains time, keyboard buffering, audible/visible heartbeat, and countdown queues. A cooperative round-robin dispatcher runs ready tasks. Player input is one task; individual creatures, damage recovery, torch burn, delayed redraw, and creature regeneration are others. An accurate core needs a clock and ordered scheduler, not just `execute(command)` state transitions. [S: COMMON `CLOCK`, `SCHED`, `QUESCN`; HUMAN `PLAYER`; COMDAT `TCBDAT`]

The active maze is a 32×32 array of four two-bit edge fields. Entering a level recreates its maze and creature population; object records persist separately with level/ownership fields. Maze construction seeds a 24-bit polynomial generator from a fixed level-indexed byte table, carves 500 cells, and adds 70 regular and 45 secret doors. **The five original map layouts are fixed across plays:** the generator advances its RNG according to the seconds counter only *after* finishing the maze. That later advance can affect creature placement and subsequent events, so identical maps do not imply identical complete game states. [S: DGNGEN `DGNGEN`, `LVLTAB`, `DGEN90`; NEWLVL `NEWLVX`; CD; COMDAT]

The Morgan game grant permits preservation-oriented reproduction subject to its original/unaltered-form condition. A separate handwritten grant in the source repository is directed to Michael Spencer for copying the listing. Neither expressly licenses the Tandy manual or all later port code/assets. This is an engineering risk to resolve before publication, especially for a modified mode or commercial release. [G; H; M]

**Gate:** implement the original timing, parser, generation, and combat as a small *behavioral reference specification plus emulator fixtures* before treating this report as permission to build a complete core. Details flagged **VERIFY** below require additional tracing or retail-ROM comparison.

## 2. Source inventory and provenance

| Source | Custodian / kind | License evidence and allowed use for this project |
|---|---|---|
| [Assembly repository][S] at `a94326f`; 1983 listing reconstructed and adapted for `lwasm` by Michael Spencer Jr. | Primary implementation evidence, with editorial reconstruction | `DAGGORATH.ASM` bears a 1983 Unified Technologies notice; the accompanying handwritten [grant image][H] gives Spencer nonexclusive permission to copy, replicate, or emulate the contained source for replicating the game. Use for behavioral study. **Do not treat the entire tree as MIT or public domain.** Recheck whether source-derived vector tables can be shipped under the broader game grant. |
| [Tandy 1983 manual][M] | Contemporary product documentation; primary intent/UX evidence | Manual bears a distinct ©1983 Tandy notice; reference and paraphrase, do not bundle its scans/text by default. OCR is imperfect. |
| [Morgan preservation grant][G], also transcribed in [3DS GRANT][G2] | Grant attributed to original DynaMicro president; primary licensing statement via later mirrors | Nonexclusive, permanent, worldwide license to develop/produce/duplicate/emulate/distribute *the game*, conditioned on every effort to preserve its original, unaltered form. Morgan describes his belief about reversion from Radio Shack, not a provided copy of the underlying contract. |
| [Hunerlach Linux/SDL port][L] at `4ab53f4`, derived from Richard Hunerlach PC port | Secondary adaptation/cross-check | Its `license/license.txt` reproduces the Morgan grant and explicitly says SDL's LGPL does not license the port. No clear standalone grant for all port C++/WAV code was established. Reference only. Its readme notes a prior erroneous allowance for climbing up holes. |
| [cognitivegears SDL2/WebAssembly core][W] at `e95bf70`, used by [daggorath.online website][WS] | Secondary adaptation | Core `license/readme.txt` says the SDL2 zlib license does not cover the game port; no comprehensive top-level code license identified in inspected checkout. Website repository is MIT, **which does not license its core submodule**. Code/WAVs reference only pending explicit rights review. |
| [3DS port][D] | Secondary input design example | Repository declares GPL-3.0. Do not copy its code into a project intended for other terms. Its handheld shortcuts are design evidence, not original command authority. |

Source attribution shorthand: **[S: filename routine]** = inspected assembly; **[M]** = original manual; **[L/W/D]** = later ports; **[I]** = engineering inference. Source listings include 2022 annotations and reconstructed missing macros, so a contradiction with original ROM must be investigated rather than assumed to be a historical bug.

## 3. Original module map and subsystem interactions

`DAGGORATH.ASM` is the assembly include spine. `CD` specifies in-memory layouts; `ONCE` initializes hardware, RAM, objects, queues, and starting level; `COMDAT`, `DTABAS`, and `TOKEN` are the initialization, entity, and lexicon tables. `COMMON` implements interrupts, scheduling, queues, and cassette I/O; `COMSWI` dispatches system services. `HUMAN` buffers/dispatches text via `PARSER`, `EXPAND`, and command handlers `PATTK`, `PCLIMB`, `PEXAM`, `PGET` (also drop/stow/pull), `PINCAN`, `PLOOK`, `PREVEA`, `PTURN` (also move), `PUSE`, `PZTAPE`. `DGNGEN`/`NEWLVL` build levels; `COMCRE`/`CRETUR` instantiate and run creatures; `COMPLR` runs player timer tasks; `HUPDAT` computes heartbeat/faint/death; `OBIRTH` creates objects. `VIEWER`, `MAPPER`, `VCTLST`, `VECTOR`, `VARC`, `VERT`, `VOBJ`, `D3`, `D4`, `SWCHAR`, `COMTXT`, `TXTSER`, `CLEAR`, `STATUS`, `PUPDAT`, `SOUNDS` implement display/audio. `D3`/`D4` are creature geometry, not game levels. [S: DAGGORATH include order; CD; other named modules]

Key interaction: interrupt moves timed TCBs to ready queue → scheduler invokes either input or creature task → handler mutates CCB/OCB/player/map → redraw/heartbeat/sound service records or displays consequences → task returns with next countdown/queue. Sound generators and animated fades can consume foreground CPU while the interrupt clock continues. Preserve this ordering when deriving exact tests. [S: COMMON; HUMAN; COMSWI; PATTK; CRETUR; SOUNDS]

## 4–5. Architecture and simulation loop

The interrupt `CLOCK` first handles display swap, opening buzz, heartbeat count/toggle/heart icon; it scans jiffy queue, increments hierarchical counters and scans queues at rollovers; finally it polls the keyboard unless fainted. `QUESCN` decrements countdowns in list order and appends expired tasks to the scheduler queue. The foreground `SCHED` loops its ready linked list, runs each task to return, then retains, reschedules, or destroys its TCB. `PLAYER` gets at most one buffered character on its turn and reschedules after one jiffy. `HUMAN` dispatches a command at carriage return; during typing creatures continue. [S: COMMON `CLOCK`, `QUESCN`, `SCHED`; HUMAN `PLAYER`, `HUMAN`]

Conceptual port model, **not a proven exact instruction-level ordering**:

```text
for each simulated 1/60-second boundary:
  perform interrupt actions in original CLOCK order
  move expiring countdown tasks to FIFO ready queue in queue/list order
  poll/admit keyboard characters unless fainted
  execute ready tasks in SCHED order until they return/yield
  retain or requeue each task with its returned countdown and time unit
  emit ordered screen and audio events during execution
```

**VERIFY:** precise `ROLTAB` values, FIFO insertion under simultaneous expirations, whether all ready work always finishes within a jiffy, blocking `SYNC`/`WAIT` and sound duration, and keyboard queue overflow. Modern `tick(elapsed)` must execute discrete historical boundaries; a large delta cannot skip intervening actions. Timestamp *keystrokes*, not only completed commands, if exact replay of typing and queue saturation matters. [I based on S: COMMON; HUMAN; CD]

## 6. Authoritative state model

| Domain | Required authoritative fields / assembly evidence |
|---|---|
| Clock/scheduler | JIFFY, TENTH, SECOND, MINUTE, HOUR, DAY; each task kind, countdown, time unit, linked/ready ordering, SLEEP and save/load control. `COMMON`, `CD`. |
| RNG/maze | Three `SEED` bytes; 32×32 maze cell bytes; current LEVEL; level/creature matrix and vertical feature index. `RANDOM`, `DGNGEN`, `CD`, `COMDAT`. |
| Player | PROW/PCOL, PDIR (0 N, 1 E, 2 S, 3 W), PPOW, PDAM, magical/physical offense/defense, carried weight, PLHAND/PRHAND, bag links, PTORCH, light, faint/freeze. `CD`, `VIEWER`. |
| Creatures | Up to 32 CCB slots with power/damage, attack/defense, movement/attack delays, object list, type, direction, row/column, active flag; per-creature TCBs. `CD`, `COMCRE`, `CRETUR`. |
| Objects | Up to 72 OCB slots; class/type/reveal requirement, owner, level/position, attack values, extra three-byte type parameters, next bag/creature link. `CD`, `OBIRTH`, `DTABAS`. |
| Input/presentation coupled state | Character ring buffer and partial line/token parsing; display mode (`VIEWER`/`EXAMIN`/`MAPPER`), map flag, pending redraw, heartbeat flags/count, temporary light and animation flags. Some flags affect input/timing and therefore **must not** be dropped as “only graphics.” `HUMAN`, `PARSER`, `CD`, `COMMON`. |

Rendered pixels, actual DAC samples, raster buffer addresses, SAM registers, and transient drawing registers can be projected presentation state in a modern port, provided events, visibility, waits, and display mode still match. Pointer identity should become stable entity IDs with ordered lists. [I]

## 7. Dungeon representation and generation

One byte per cell: low to high two-bit pairs north/east/south/west; `00` passage, `01` regular door, `10` secret door, `11` wall. Coordinates are row/column 0–31; `MAP32` uses `row*32+col`; `BORDER` prevents out-of-range neighbors. `STEP` uses north `(-1,0)`, then clockwise. `VFTTAB` gives fixed ladder/hole coordinates by level. Both sides of a created door are marked. [S: DGNGEN; CRETUR `STEP`; COMCRE `VFTTAB`]

`DGNGEN` fills 1,024 bytes with `0xFF`; initializes the 24-bit seed from `LVLTAB + LEVEL` (overlapping three-byte windows, **not** three disjoint bytes per level); starts at a pseudorandom cell; selects a pseudorandom direction and run length 1–8; walks until 500 fresh cells have been cleared, rejecting bounds/corner patterns; derives wall bits from neighboring cells; and inserts 70 regular and 45 secret doors at eligible pseudorandom edges. Because the seed is fixed per level, this procedure recreates the same five historical maps. **Only after all maze cells and doors are complete** does `DGEN90` spin the RNG according to `SECOND`, changing subsequent draws rather than the map just built. `NEWLVL` resets CCBs/tasks, builds the maze, populates creatures from 5×12 `CMTTAB`, and distributes creature-owned objects. It changes display inversion for alternate levels. [S: DGNGEN `DGNGEN`, `MAKDOR`, `DGEN90`; NEWLVL; COMDAT]

**VERIFY:** exact corner rejection arithmetic, level return/recreation, map door traversal semantics, `RNDCEL` draw order, vertical feature destinations, and any difference between the reconstructed listing and retail ROM. Original Mode must reproduce the five fixed layouts and historical level-entry behavior; no user-specified seed belongs in its game setup. Alternate seeds or newly generated maps belong only in an explicitly separate enhanced/test mode. [I]

## 8. Commands and parser

The *shipped command table* contains 15 verbs; `RESTART`, `SETOPT`, `SETCHEAT` and a bare `BACK` are **not** in this original `CMDXXX`. Later port commands must stay out of Original Mode. `DIRXXX` spells `BACKWARD`, although the manual uses `BACK` and unique-prefix matching accepts it. The manual says ambiguous prefixes and impossible commands print `???`; its ability to queue rapidly typed commands is material to timing. `PARSER` compares compressed table entries and rejects multiple prefix matches; `INCANT` additionally demands an entire magic adjective (`FULFLG`). Generic or adjective-plus-matching-class object syntax is supported. [S: DTABAS `CMDXXX`, `DIRXXX`; TOKEN; PARSER; PINCAN; M]

| Original verb | Syntax / typed command | Key semantics and failure/output evidence |
|---|---|---|
| MOVE | `[BACKWARD|LEFT|RIGHT]`, default forward | Step/sidestep; blocked moves still run weight-based exertion path; half-step animation for forward/back. `PTURN:PMOVE`. |
| TURN | `LEFT|RIGHT|AROUND` | Facing change; visual sweep and screen synchronization. `PTURN`. |
| CLIMB | `UP|DOWN` | Requires suitable feature; up only on upward ladder; holes only down. `PCLIMB`; M. |
| EXAMINE | none | Switch to room/creature/floor/backpack text view. `PEXAM`. |
| LOOK | none | Return to forward view; manual says use after EXAMINE. `PLOOK`; M. |
| GET | `LEFT|RIGHT` + item | Empty hand and matching floor object; updates carried weight/heartbeat. `PGET`. |
| PULL | `LEFT|RIGHT` + item | Empty hand and item in bag; removes selected lit torch from active torch if pulled. `PGET:PPULL`. |
| STOW | `LEFT|RIGHT` | Hand object to bag; linked-list insertion; does not shed carried weight. `PGET:PSTOW`. |
| DROP | `LEFT|RIGHT` | Hand object to current floor; reduces weight. `PGET:PDROP`. |
| ATTACK | `LEFT|RIGHT` | Empty hand allowed; incurs self-exertion even without target; strike checks current cell, hit and dark penalties, damage/death/reward. `PATTK`. |
| USE | `LEFT|RIGHT` | Torch lights then stows; specific flasks/scrolls dispatch; unsupported object use returns without effect. `PUSE`. |
| REVEAL | `LEFT|RIGHT` | Held object reveals when `25 * reveal_requirement <= power`; otherwise no change. `PREVEA`. |
| INCANT | full magic adjective | Checks rings in both hands, transforms on matching hidden word, final ring triggers victory; no partial spell. `PINCAN`. |
| ZSAVE / ZLOAD | filename token | Original cassette game image and interrupt-mediated save/load, not filesystem slot commands. `PZTAPE`; `COMMON:SAVE/LOAD`. |

The known short forms follow unique matching (`A L`, `M B`, `G R T`, `P L SW`, `I STEEL` are examples from the manual/website). **Do not hardcode one abbreviation per verb**: derive the full abbreviation set dynamically from the actual tables, test ambiguous cross-table terms, and verify handling of extra trailing tokens and overlong input. The primary `TOKEN` compressed strings and the manual should settle every adjective, including magic names. Parser failure usually prints `???`, but some handlers silently return (for example INCANT/REVEAL/USE), so failure handling is command-specific. [S: TOKEN; PARSER; handlers; M]

Typed core boundary proposal: `ParseResult {command, originalText, parseStatus}` → semantic command handlers, with `Command` variants for each verb and `Hand`, `RelativeDirection`, `ObjectQuery{class, optional adjective}`, `MagicWord`, `SaveName`. Keep a separate timed keystroke adapter for exact historical replay. [I]

## 9. Creatures and AI

Source order maps creature indices to sounds and the five level population rows. Times are **tenths of a second**, subject to scheduler delays. `power` is the CDB attack/health baseline; four offense/defense bytes are radix-7 factors, not conventional percentages. [S: DTABAS `CREXXX`/`CDB`; COMDAT `CMTTAB`; SOUNDS]

| Type index | Source type | Power | Move / attack tenths | Magic off/def; physical off/def | Initial counts L0–L4 |
|---:|---|---:|---|---|---|
| 0 | Spider | 32 | 23/11 | 0/255; 128/255 | 9,2,0,0,2 |
| 1 | Viper | 56 | 15/7 | 0/255; 80/128 | 9,4,0,0,2 |
| 2 | Stone giant I | 200 | 29/23 | 0/255; 52/192 | 4,0,0,0,2 |
| 3 | Blob | 304 | 31/31 | 0/255; 96/167 | 2,6,4,0,2 |
| 4 | Knight I | 504 | 13/7 | 0/128; 96/60 | 0,6,0,0,2 |
| 5 | Stone giant II | 704 | 17/13 | 0/128; 128/48 | 0,6,6,0,2 |
| 6 | Scorpion | 400 | 5/4 | 255/128; 255/128 | 0,0,8,8,2 |
| 7 | Knight II | 800 | 13/7 | 0/64; 255/8 | 0,0,4,6,4 |
| 8 | Wraith | 800 | 3/3 | 192/16; 192/8 | 0,0,0,6,4 |
| 9 | Balrog (manual: Galdrog) | 1000 | 4/3 | 255/5; 255/3 | 0,0,0,4,8 |
| 10 | Wizard image | 1000 | 13/7 | 255/6; 255/0 | 0,0,1,0,0 |
| 11 | True wizard | 8000 | 13/7 | 255/6; 255/0 | 0,0,0,0,1 |

`CMOVE` priorities: if frozen, skip; dead → remove task; most non-scorpion/non-wizard types pick up a local object first (one per action); else attack if in player cell; else if row/column aligned and all intervening steps traversable, face and move toward player; else select forward/left/right or forward/right/left preference from a random byte, putting a side turn first 25% of the time, then try backward. Requeue at movement or attack delay based on occupancy. `CREGEN` runs on a five-minute schedule and increments an RNG-selected type 2–9 in the level matrix if the current total is under 32; inspect its separate creation path before asserting *when* new creatures become visible. Preserve this timed rule and the original initial level populations, rather than adding independent spawn schedules. [S: CRETUR `CMOVE`; COMCRE `CREGEN`; NEWLVL; COMDAT `CMTTAB`]

## 10–12. Combat, items, magic, and timing

`PATTK` copies held object's offensive values (empty hand uses `EMPHND`), computes an exertion increment from offense and current power, adds it to `PDAM`, plays the object's sound, uses ring charge when appropriate, and only then searches for a creature in the same square. Ordinary attacks use `ATTACK` probability; a dead/unlit torch applies an additional 1-in-4 success gate; a ring bypasses ordinary hit and darkness checks. `DAMAGE` adds magical and physical channels separately: `scale16(scale16(attackerPower, attackFactor), defenseFactor)` each, where factors are radix-7 and 16-bit operations matter. `ATTACK` compares defender remaining power (`power−damage`) against attacker power in quarter steps, adjusts an 8-bit RNG outcome with reward/penalty, and tests threshold 127. **VERIFY** exact byte wrapping and signed flags in a compiled fixture. Kill drops loot, decrements population, grants one eighth of victim power up to about 32K, then branches on wizard identity. [S: PATTK `PATTK`, `ATTACK`, `DAMAGE`, `SCAL16`]

Player has power and accumulated damage rather than an independent stamina meter. MOVE adds `(carriedWeight >> 3) + 3` damage even on the blocked path. ATTACK adds damage as effort; `HSLOW` recovers approximately one sixty-fourth of accumulated damage per heartbeat-based interval; `HUPDAT` derives heartbeat delay as `(P*64)/(P+2D)−19` with 24-bit arithmetic and a source-specific subtract loop. Faint at heartbeat delay ≤3; recover when >4; keyboard scanning is suspended during faint; death if damage exceeds power. Audible heartbeat toggles a bit at its jiffy countdown; visual heart update can be disabled in map mode while audio timing is separately flagged. [S: PTURN `PMOVE`; COMPLR `HSLOW`; HUPDAT; COMMON `CLOCK`]

`OBJXXX` catalogs six classes. Nominal inventory is a linked bag with **no explicit per-bag slot cap found**; global OCB table has 72 slots and burden grows from class weights (flask 5, ring 1, scroll 10, shield 25, sword 25, torch 10). Initial bag: wooden sword and pine torch. Objects have hidden specificity until revealed, class appearance sometimes initialized via a generic template. Source items: swords wooden/iron/elvish; shields leather/bronze/mithril; torches pine/lunar/solar/dead; flasks THEWS/HALE/ABYE/EMPTY (handler comments identify strength/healing/poison but aliases differ); scrolls vision/seer; ring labels VULCAN/HOTH/JOULE/SUPREME and transformed FIRE/ICE/ENERGY/FINAL/GOLD. Do not infer player-facing magic words solely from these labels: inspect `TOKEN.ASM`. `BURNER` runs each minute, decreases active torch timer, changes to dead at its threshold (code tests ≤5), and lowers regular/magic light via extra parameters; manual advertises nominal 15/30/60-minute lifetimes. **VERIFY** exact end-of-light curve and why the dead threshold is five. [S: DTABAS; COMDAT; OBIRTH; PUSE; COMPLR; M]

`USE` strength flask adds 1000 power, healing sets damage to zero, poison adds approximately 80% power to damage, and each leaves an empty flask. Revealed vision/seer scroll selects map without/with features; unrevealed scroll refuses its effect. Rings transform by `INCANT` when held and a full matching magic word is entered. This is the behavior of source dispatch routines, with item naming conflicts left open pending manual/ROM checks. [S: PUSE; PINCAN]

## 13. Randomness and replay

`RANDOX` is a 24-bit shift-register/polynomial implementation: eight rounds each count feedback bits selected by `0xE1` from `SEED+2`, rotate the count's parity into three seed bytes, and return `SEED[0]`. Preserve byte widths, shift order, and call count; substituting a generic seeded generator changes historical outcomes. The fixed level seeds create the canonical maps; the `SECOND`-dependent spin happens afterward and affects the subsequent RNG stream. Deterministic replay of creature placement, behavior, and event order needs clock counters at level entry, task ordering/countdowns, complete entity and RNG state, and timestamped input; `seed + command list` alone is insufficient. The original game is not a free-seed random-map game. Noise generation in `SOUNDS:SNOISE` should be audited for whether it consumes `SEED` and thereby changes future gameplay. [S: RANDOM; DGNGEN `DGEN90`; SOUNDS; I]

## 14. Rendering

`VIEWER` projects from PROW/PCOL/PDIR in increasing RANGE, parses four two-bit edge features, draws architecture, creatures, vertical features and objects subject to regular/magical light, and stops at blocked line of sight. `NORSCL`/`HLFSCL`/`BAKSCL` provide fixed-point scale tables; `VCTLST` traverses encoded vector lists; `VECTOR` uses a digital differential analyzer with fades; geometry lives in `VARC`, `VERT`, `VOBJ`, `D3`, `D4`. `MAPPER` and `EXAMIN` are distinct display modes. Original screen uses 256-wide coordinates, display centroid `(128,76)`, top viewport to scan line 152, status to 160, command region to 192; flip/flop buffers and SAM/VDG inversion are hardware details. TURN and MOVE add foreground animation and `SYNC` waits that can affect game timing. [S: VIEWER; VCTLST; VECTOR; CD; COMDAT; PTURN; PUPDAT]

SDL recommendation [I]: decode original vector data into immutable model coordinates; preserve scale, clip, draw order, lighting, and timed transitional frames; draw to a logical 256×192 surface and scale to mobile safely. Keep original font/text positions and selectable integer/pixel geometry. Verify vector output against emulator screenshots at fixed states before choosing antialiasing defaults. Rendering need not expose raw VRAM, but command blocking and display-mode transitions belong in the simulation/event contract.

## 15. Audio

`SOUNDS:SNDTAB` maps 12 creature types, six item classes, hit/wall/explosion effects to DAC waveform generators; `CLOCK` separately toggles heartbeat output on `HEARTC`, and opening buzz uses a 30 Hz toggle. Creature sound volume depends on position/range; a same-cell creature explicitly requests maximum volume. Audio conveys nearby threats and player condition; preserve source event order, location/volume cues, and pulse rate. Original routines generate waveforms during synchronous execution, while later PC ports contain WAV recordings whose provenance is separate. **VERIFY** exact distance attenuation, whether sound generation's use of CPU or PRNG changes scheduling, and whether a port's WAVs match retail output. [S: SOUNDS; COMMON `CLOCK`; CRETUR; M; L]

## 16. Progression, death, victory

Five populated levels are in `CMTTAB` (indices 0–4). Level 2 contains a wizard image in the table. Killing that type in `PATTK` triggers `ENDGAM`: dialogue, strips equipment except a torch, imposes weight 200, regenerates level 3 and chooses a random player location. Killing the true wizard on level 4 freezes creatures and sets up the final ring sequence; `PINCAN` with the final ring's full word reaches the winner screen. Death follows `HUPDAT` when damage exceeds power. Original save/load uses cassette and carries game state in RAM blocks; a modern save format needs to include scheduler and RNG state, not just position. [S: COMDAT; PATTK `ENDGAM`; PINCAN `WINNER`; HUPDAT `DEATH`; COMMON `SAVE/LOAD`]

## 17. Quirks and discrepancies ledger

| Observation | Classification | Action |
|---|---|---|
| Manual says “Galdrogs”; source type name `BALROG`. | Documentation/source naming difference | Preserve displayed historical term after ROM/manual comparison. |
| Manual suggests `CLIMB` and `CLIMB UP`; source rejects empty direction. | Source/manual discrepancy | Emulator test empty `CLIMB`. |
| Later port previously allowed ascending holes; readme explicitly calls this erroneous. | Confirmed port bug, not original rule | Assert up-hole failure in Original Mode. [L] |
| `LVLTAB+LEVEL` overlaps three seed bytes. | Source implementation artifact, likely intentional compression | Preserve; test hashes for all five levels. |
| Blocked MOVE still adds exertion. | Source behavior, likely intentional | Retain pending ROM check. |
| Creature may collect an object in its cell before attacking the co-located player. | Source behavior, likely intentional | Regression fixture. |
| Source listing has 2022 `lwasm` compatibility edits/rebuilt macros. | Evidence limitation | Compare binary/ROM before claiming byte-exact fidelity. |
| Website labels MIT, while its engine is a separately licensed/undetermined submodule. | Licensing scope distinction | No engine code copying on website license alone. |

## 18. Licensing and provenance policy

The broad Morgan text authorizes reproduction of **the game** conditioned on preservation and states a belief about rights reversion. It does not explicitly resolve ownership of all contributors' art/sound, Tandy's manual, trademark use, later port copyrights, or commercial terms. The handwritten image in the assembly checkout is a **different, named grant** to Spencer concerning the contained source code; it does not by itself appoint all downstream recipients to relicense source. Original assembly's 1983 notice names Unified Technologies whereas the manual and grant refer to DynaMicro/Tandy: record the discrepancy for counsel. [G; H; S: DAGGORATH; M]

Every incoming file/asset receives `origin URL + pinned commit + original author + rights evidence + target use + copied/adapted/reference-only + reviewer`. Treat original source/manual and ports as behavioral references; write independent core code. If deliberately incorporating historical vector or audio data, isolate it as a separately tracked licensed artifact and document reliance on the Morgan grant or obtain specific permission. GPL-3DS code and unidentified PC/Web core code are reference-only. Keep original and future extensions separate and seek legal review of public distribution, title/branding and monetization before release. [I]

## 19. Proposed modern boundaries

`historical-data` (versioned tables, vector geometry, lexicon, provenance); `clock-scheduler` (60 Hz, stable task order, key queue, waits); `world` (maze and vertical features); `entities` (stable IDs, level, ownership, ordered lists); `rules` (commands, creature tasks, combat, physiology, generation, RNG); `presentation-state` (visibility, modes, transitions, textual output and timestamped sound cues); adapters for CLI tests, SDL display/audio, then touch/platform save. A C++20 core should expose `advanceJiffies(n)` and `feedKey(key, atJiffy)` plus state/event snapshots, and optionally semantic command injection for unit tests; semantic injection alone cannot certify input fidelity. [I]

## 20. Conformance testing

1. Build a retail-ROM emulator harness with fixed recorded keystrokes, screenshots, audio timing and RAM watches, if legally obtainable. Capture exact listing revision and ROM hash; separate *source-derived* expectations from observed ROM outcomes.
2. Golden fixtures: one canonical maze cell hash for each level 0–4, unchanged at different entry times and on level re-entry; RNG state immediately before and after the `SECOND`-dependent post-maze spin; starting player/bag and initial creature populations; line-of-sight and door traversal; parser exhaustive unique prefixes; same-cell combat; attacks in darkness; torch minute boundary; faint/recovery/death; object-before-attack priority; creature timer ties; five-minute regeneration boundary; level change and both wizard endings.
3. Differential traces after each jiffy/command: player/creature/item records, queues, RNG, render state and ordered sound events. Report earliest divergence, never silently update snapshots.
4. Property checks: reciprocal door edges, bounded indices, entity ownership uniqueness, deterministic repeat with identical complete initial state and timestamped input. Screenshots test geometry/text, not mechanics. [I]

## 21. Open questions and evidence needed

| Priority | Question | Proposed resolution |
|---|---|---|
| Blocker | Exact `ROLTAB`, queue insertion/tie ordering, blocking sound/animation and input overflow? | Trace `COMMON`, `ONCE`, `SOUNDS`, `HUMAN`; instrument emulator at 60 Hz. |
| Blocker | Are assembly listing/lwasm fixes behavior-equivalent to retail ROM? | Assemble, compare cartridge hash, document differences. |
| Blocker | Exact parser aliases, trailing tokens, full ring incantations and invalid-command time? | Decode `TOKEN` tables, enumerate prefixes, ROM command probes. |
| Blocker | How does `CREGEN` matrix increment become a live creature, including on return to a level? | Trace `COMCRE`, `NEWLVL`, test 5-minute transition. |
| High | Do map bytes remain identical for every level-entry time and each return in the retail ROM; which later placements depend on `SECOND`? | Hash all five maps across timed emulator entries; compare post-maze RNG, creature positions, and level-return traces. |
| High | Full effects and edge cases for doors, vertical features, darkness, shield filters, 16-bit overflow? | Focused source walkthrough plus fixtures. |
| High | Are manual's empty CLIMB and item names inconsistent with game output? | Emulator and manual scan comparison. |
| High | Does DAC noise consume gameplay RNG; exact distance sound mapping? | Trace `SOUNDS:SNOISE` and callers. |
| Distribution | Rights for assembly-derived geometry/sound, original art/manual, name and extended mode? | Preserve evidence; qualified legal review before publishing. |

## 22. Recommended next milestone

Produce a **Phase 0b executable evidence pack**, with no mobile UI: (a) pin source/ROM versions; (b) extract canonical maze hashes for all five levels and decode token, item, and creature tables into machine-readable fixtures; (c) write a jiffy-by-jiffy scheduler/clock specification with simultaneous-event ordering; (d) capture emulator traces for timed spawns, level entry/return, and the blocker questions; (e) update this report from “VERIFY” to proven or explicitly variant-specific behavior. Then implement a minimal headless C++20 slice: original RNG, fixed level-0 generation, timed keyboard parsing, MOVE/TURN/LOOK, and a trace-comparison CLI. Only expand to combat after that slice matches the emulator. [I]

## References

[S]: https://github.com/MichaelSpencerJr/DungeonsOfDaggorath/tree/a94326f00ebb16a106b540c58bc2ccf5f7b66dac
[H]: https://github.com/MichaelSpencerJr/DungeonsOfDaggorath/blob/a94326f00ebb16a106b540c58bc2ccf5f7b66dac/grant_of_license.png
[M]: https://archive.org/stream/Dungeons_of_Daggorath_1983_Tandy/Dungeons_of_Daggorath_1983_Tandy_djvu.txt
[G]: https://iloveglory.freehostia.com/daggorath/license.html
[G2]: https://github.com/pyroticinsanity/3dsdod/blob/master/GRANT
[L]: https://github.com/gondur/dungeons-of-daggorath/tree/4ab53f4b4478df028d14a403fc03d9ff5266cde1
[W]: https://github.com/cognitivegears/DungeonsOfDaggorath/tree/e95bf70c6e3101ecd3b954fd6fdbdf945e8ef2f4
[WS]: https://github.com/DungeonsOfDaggorath/DungeonsOfDaggorath.github.io
[D]: https://github.com/pyroticinsanity/3dsdod
