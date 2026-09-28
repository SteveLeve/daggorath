# Phase 5 reconciliation

Labels: **[SRC]** read from the pinned listing (also written "Source-proven"); **[INF]** inferred; **[OPEN]** unresolved; **Core-observed**: seen in a headless core trace, not a ROM capture. Sections before 2026-09-27 contain no ROM observations; later sections rely on D-19's ROM-measured build timing (capture C-22), which is labelled in §13.

`ENDGAM`, the ring riddle, and `WINNER` replace the `DEFER` stubs from Phases 3 and 4 (D-8 and D-9 are retired in `clock-and-scheduler.md` §13). No verb reports `UNIMPLEMENTED`.

## Settled

| Item | Label | Disposition |
|---|---|---|
| Dialogue text | [SRC] | `tools/decode_outsti.py` decodes every `OUTSTI` string. `DEATH`, `ENDGAM`, and `WINNER` emit their exact strings, including the leading carriage return and the trailing `...`. The earlier core printed the death line without its periods, and the endings printed nothing. |
| `ENDGAM` hands | [SRC] | `ENDGAM` only replaces `BAGPTR` with `PTORCH`. The earlier core also cleared both hands. |
| What `ZSAVE` writes | [SRC] | The whole `DP.BEG`–`MM.END` range (ADR-0005). The earlier core stored a 20-field subset and dropped creatures, objects, queues, and part of the clock. `ram_image()` now holds every modeled field, and `ZLOAD` resumes at the exact post-save state. |
| When the tape runs | [SRC] | `SCHED1`, after the `PLAYER` task is requeued. `LOAD90` then clears `ZFLAG`, runs `INIVU`, and ends the lap. |
| `FNDCEL` order | [SRC] | `RNDCEL` draws the column, then the row. |
| Suspend snapshot | — | `snapshot()` round-trips byte for byte, and a replay after restore matches the original run line for line, including into a game whose level, clock, and halt state had diverged. |

## Trace baseline change

`docs/archaeology/phase-3/traces/fight-to-death.trace` gains one line on this branch: `1759 0:0:35.6.0 DIALOGUE ^ YET ANOTHER DOES NOT RETURN...`, the `DEATH` `OUTSTI` string that Phase 5 emits. No other line changes. The trace is regenerated here for that reason only. `fight-to-kill.trace` is unchanged.

## Tests

`tests/conformance/progression_regressions.cpp`: both endings, `WINNER`, the death line, `ZSAVE`/`ZLOAD` resuming at the save point with creatures moving, an absent name (D-11), the RAM image restoring byte for byte into a different game, and the snapshot round trip plus replay.

## Deviations

| Id | Owner |
|---|---|
| D-11 `ZLOAD` of an absent name reports `???` | Permanent, Original Mode cassette substitute (ADR-0005) |
| D-12 `FUDGE incoming` / `FUDGE rest` | **Retired 2026-09-28.** Not source behaviour; used only by the superseded 2026-09-26 route. Removed from the core, `dcli` and the script parser (see "2026-09-28 — FUDGE removed; desktop uses the core restart"). |
| D-1, D-2 lap model | ADR-0002, Track R |
| D-3 `HSLOW` zero-countdown clamp | Track R |
| D-4 animation and sound durations | Phase 6 (listing-derived part), Track R (measured part) |
| D-5 trace counter sampling | Permanent, trace format only |

## Playthrough harness (historical; not source behaviour, superseded 2026-09-28)

A closed-loop planner (`src/app/dplan.cpp`) records a power-on keystroke script.
A pure power-on run can take hours of HSLOW recovery and dies to a single
scorpion sting at Original Mode incoming damage. The 2026-09-26 route used two
harness mechanisms so a committed script could still be replayed by `dcli` /
ctest; both were removed on 2026-09-28 (see below) and neither applies to the
committed script:

1. **Checkpoints.** After each cleared level, and before the type-10 and
   type-11 fights, `dplan` writes `Game::snapshot()` to
   `.cache/playthrough/<stage>.snap` (gitignored) and types a real
   `ZSAVE <stage>` so `dcli` can `ZLOAD`. The keystroke log beside the snap
   stays one file from power-on.
2. **Fudge factor (D-12).** Script lines `FUDGE incoming <percent>` and
   `FUDGE rest` are not command-parser tokens. `dcli` and `dplan` apply them
   through `Game::set_incoming_damage_percent` and `set_player_damage(63)`.
   100 is Original Mode; 25 is the proof setting.

These are labeled **not source behaviour**. They do not change Original Mode
combat unless a `FUDGE` line is replayed.

**Superseded 2026-09-28 (Phase 5b).** The planner no longer uses either
mechanism for the committed script. It emits no `FUDGE` lines; its checkpoints
are typed `ZSAVE`s confirmed by the trace, and a death is recovered with a
keypress RESTART and a typed `ZLOAD`, never a `.snap` restore. `Game::snapshot()`
is used only on separate scratch `Game` copies for lookahead search, which never
supply the candidate's state. The D-12 harness itself was removed on 2026-09-28.
See "Playthrough status" and "2026-09-28 — Phase 5b honest baseline".

## Open

| Item | Label |
|---|---|
| `OCBPTR` rewind in the ring riddle | [OPEN] The core has no object-allocation pointer, and nothing allocates an object after the riddle. |
| Drawing the death and winner screens | Phase 6 |

## Playthrough status

Authored by `src/app/dplan.cpp` (Phase 5b, commit `c826dec`). Committed script:
`docs/archaeology/phase-5/traces/power-on-to-winner.script`. `ctest -R
playthrough_power_on_to_winner` runs `tools/verify_playthrough.py`: two fresh
default-startup `dcli` replays must be byte-identical, pass the full-trace
checks (no `FUDGE`, unique typed saves, every death followed by RESTART and a
ZLOAD of the latest save, exactly one WINNER) and match the recorded digest.
Replacement reason: see "2026-09-28 — Phase 5b honest baseline" below.

| | |
|---|---|
| Keys | 179036 timestamped keystrokes; no `FUDGE` or other harness lines |
| Jiffies at `WINNER` | 381122 (planner `won=1` at 381124) |
| Saves | 8 typed `ZSAVE`: POWERON, FLOORA, FLOORB, FLOORC, IMAGE, ENDGAM, FLOORD, WIZARD |
| Deaths | 6, each followed by the keypress RESTART and a typed `ZLOAD` of the latest save |
| Replay length / timeout | `--jiffies 382000` (the trace's last event is at 381139); 120 s per replay (measured 8–15 s) |
| Two-replay sha256 | `b56777eecb2c887c565bd0b55add00ed099e9ac085f3c3e6d28b0e7b753661d1` |

Two matching core traces prove deterministic core replay under the recorded
deviations, not ROM conformance.

### Superseded baseline (2026-09-26, FUDGE)

| | |
|---|---|
| Jiffies at `WINNER` | 190875 (clock `0:53:7.5.2`), re-planned 2026-09-26 after the HSLOW correction |
| Final damage | 63 (trace `# final` line; the last `FUDGE rest` set it) |
| Final cell | row 14, col 6, dir 1 (trace `# final` line) |
| `FUDGE incoming 25` | one line, jiffy 90400 (D-12, not source behaviour) |
| `FUDGE rest` | 2290 lines (D-12, harness). Without `--fudge` the planner clears levels 0-2, then dies on level 3 |
| Two-replay sha256 | `be5409e516d2d77cb38cba2bb83ee5086fffbdbfada438c9ccec8c2235be0ead` |

| Stage | Fudge | Notes |
|---|---|---|
| Power-on through cleared level 0 | 100 | Wooden/iron, VULCAN → FIRE. Checkpoint `cleared-0`. |
| Cleared level 1 | 100 | RIME → ICE, bronze, revealed IRON in hand. Checkpoint `cleared-1`. |
| Level 2 scorpions | 100 | Revealed IRON (pho 40) under a live torch, hit-and-run. Image left alive. Checkpoint `cleared-2` / `pre-image`. |
| Type 10 / ENDGAM | 25 | Original incoming one-shots the landing. `FUDGE incoming 25` plus `FUDGE rest`. Checkpoint `endgam`. |
| Cleared level 3 | 25 | JOULE → ENERGY, ELVISH. Checkpoint `cleared-3`. |
| Level 4 | 25 | Revealed ELVISH kite; never climb an occupied hole; ENERGY hit-and-run on type 11. The later combat audit below corrects the original claim that zero physical defense makes the Elvish sword ineffective. `GET` SUPREME, `INCANT FINAL`. |

## Correction 2026-09-27: rings have three charges; playthrough disabled

**[SRC]** `OCBFIX` writes `P.OCXXX` only when the type has an `XXXTAB` entry (`OBIRTH.ASM:73-85`). `INCANT` therefore keeps VULCAN's charge count of 3 on the fire ring. The core zeroed it, so the count wrapped and the ring lasted 256 swings. When the last charge is spent, `PATTK` stores `T.RN20` and calls `PREV00` (`PATTK.ASM:44-54`, `PREVEA.ASM:34-38`). That runs `OCBFIL`, so the gold ring gets its own 0/5 offense, and it clears `P.OCREV`. The core kept 255/255, so a spent ring still hit every time at full force. Both are fixed.

The committed `traces/power-on-to-winner.script` and its two-replay sha256 `be5409e5…0ead` in the table above are **superseded, not regenerated**. That route won by swinging the fire ring hundreds of times, which the listing does not allow. The conformance route test's "572 rested fire-ring swings kill the wizard" figure came from the core, never from a ROM run, and has been replaced by a 3-charge, gold-ring check. Re-running `dplan --fudge` stops with "rings not ready" at the level-2 image, because its strategy assumes a lasting fire ring. `ctest` `playthrough_power_on_to_winner` is **disabled** (`tests/CMakeLists.txt`) until the planner is reworked and the script re-authored, as the project owner decided on 2026-09-27 (refinement log Q3).


## 2026-09-27 — Phase 5b headless recovery slice

**Source-proven:** HUPDAT DEATH loops in the foreground while CLOCK continues;
CLK50 redirects a keypress to GAME, whose COMINI clears RAM and the triggering
key. The core now preserves cassette, future keys, trace history and external
replay time while resetting gameplay state. Typed ZLOAD resumes the save.
See the scheduler specification's Phase 5b restart section and D-18 for inferred
startup timing. WINNER likewise leaves interrupts running, with no death restart.

Verification: all 13 enabled ctests pass; WINNER replay remains disabled. Two
fresh default dcli runs of a temporary 1700-jiffy recovery script match byte for
byte: ZSAVE QUEST at 12, natural DEATH at 1591, keypress RESTART at 1601, typed
ZLOAD QUEST at 1622. No harness directives or startup overrides were used.
Independent evidence/boundary review found only the collateral victory-clock
assertion, now corrected and documented. All 56 fixtures verify. No fixture,
trace baseline or manifest was regenerated. Future death trace regeneration may
change post-death IRQ events because the former full interrupt halt was wrong;
review generated changes against this recorded reason before acceptance.

The desktop target builds and existing dod/dcli parity test passes; its death
menu and filesystem load shortcut remain deferred. No honest WINNER route is
established; planner and independent acceptance implementation remain pending.

## 2026-09-27 — Wizard1 weapon-channel correction and route status

**Source-proven:** `DTABAS.ASM` gives WIZ1 `MGO=255, MGD=6, PHO=255,
PHD=0`; the Elvish sword has `MGO=64, PHO=64`. `PATTK.ASM DAMAGE` applies
magic and physical offense/defense as separate `SCAL16` channels. At player
power 10660, the Elvish sword's magic channel deals
`SCAL16(SCAL16(10660,64),6) = 249`; its physical channel deals zero. Therefore
“wizard `pdef` 0 zeros a sword” was false: it described only the physical
channel. The secondary walkthrough's finite-ring-then-sword advice is
consistent with this result, but does not establish safe timing or survival.

**Core-observed:** a temporary `dcli` trace recorded an Elvish-sword `HIT` on
WIZ1 and a 249-point increase to the wizard's damage at that power. This is a
headless core trace, not a ROM capture. A planner candidate reached WIZ1 with
power 10660 and two useful rings; after the rings ran out it accumulated 5473
WIZ1 damage before the player died through the bounded recovery attempts.
These exploratory runs have no committed candidate script, accepted trace, or
hash. They show that the sword can contribute damage, not that a safe sword
route has been established.

Gameplay-model fixes from the active `refinement/discovery-2` checkout were
carried into the dedicated worktree with their regression tests and evidence:
death clears FAINT, BURNER's redraw is charged, ENDGAM and CLIMB level-build
timing follows the C-22 measurements, and the starting bag's `$0B` level byte
is preserved. The BURNER test correction and the related EXAMINE scrolling fix
were included to keep the imported test/evidence set coherent. `make all` passes;
it regenerates no fixture, trace baseline, or manifest. The power-on WINNER
test remains disabled and Phase 5b remains open until a candidate passes the
prompt's independent two-replay acceptance checks.

## 2026-09-28 — Phase 5b honest baseline

**Reason for replacing the baseline.** The 2026-09-26 script depended on the
fire-ring charge wrap that the 2026-09-27 ring correction removed, and on
`FUDGE incoming 25` and 2290 `FUDGE rest` lines (D-12, harness). It was
superseded, not regenerated, on 2026-09-27 and its ctest disabled. The
replacement was authored from scratch by the reworked planner under the Phase 5b
prompt: no `FUDGE`, 100% creature damage, progress kept with typed `ZSAVE`, and
deaths recovered through the core's source-proven AUTFLG restart (D-18) and a
typed `ZLOAD` of the latest save. No expected value was edited to make a
comparison pass: the digest is computed from the candidate's own two replays.

**Route search** (`docs/planning/phase-5b-search-log.md`, runs 0–3). After
the D-19 timed level builds were merged, older candidates no longer replayed.
Three planner changes then reached WINNER: waiting out the timed CLIMB and
ENDGAM builds; clearing level 4 instead of resting in LightUp; and finishing
WIZ1 with the Elvish sword once the rings were spent, then taking SUPREME after
queued keys settle. The replay verifier gained the `NEWLVL` event that D-19
builds emit.

**Source-proven:** within a level, creature damage (P.CCDAM/P.ATDAM, offset
10) is changed only by `PATTK.ASM DAMAGE` (`PATTK.ASM:343-358`), which adds to
it; `NEWLVL`'s `SWI ZERO` (`NEWLVL.ASM:30-33`) clears the CCB table when a level
is built, and a `ZLOAD` restores RAM. So WIZ1 does not heal between exchanges.
`PCLIMB.ASM PCLI10` (`:36-38`) needs `VF.LUP` to climb up, and `COMCRE.ASM`
`VFTTAB` gives level 4 only hole-up entries (code 0, `:198-201`), so there is no
retreat upstairs from level 4.

**Core-observed** (committed route): WIZ1 is fought at player power 9660. Four
charged-ring hits add 902 each (3608), then twenty Elvish-sword hits add 226
each, taking WIZ1's damage to 8128 against its power 8000. The kill adds 1000
power (`PATTK.ASM:132-137`, creature power ÷ 8), giving the recorded peak of 10660. Candidate qualification: `python3 tools/verify_playthrough.py --dcli
build/src/app/dcli --script docs/archaeology/phase-5/traces/power-on-to-winner.script
--jiffies 382000 --timeout 120` gives the digest above; at `--jiffies 381124`
the shorter trace gives `1628c7c8c99e2cb3c10981ecca7f53ac437a3b7fd299fbad310b77783cd87be4`.

Headless core/dcli recovery is what this baseline exercises; the desktop's own
death menu was replaced by the core restart afterwards (next section).

## 2026-09-28 — FUDGE removed; desktop uses the core restart

**D-12 retired.** Nothing but its own test and the superseded script used the
`FUDGE` harness. `Game::set_incoming_damage_percent`, `load_harness`,
`parse_harness` and the creature-hit scaling are removed; `parse_script` now
rejects a `FUDGE` line instead of skipping it, so the superseded script (kept in
git history) no longer replays. `DAGSNAP 1` keeps its layout: the retired field
is written as 100 and ignored on restore (regression test in
`progression_regressions.cpp`). The scaling only ran for a percent other than the
default 100, which Original Mode never set, so removing it cannot change Original
Mode play; the unchanged Phase 5b baseline digest is consistent with that. `set_player_damage` is a separate test
set-up call and stays.

**Desktop death.** The desktop no longer shows `R RESTART OR L LOAD`, rebuilds the
`Game`, or calls `restore_ram_image` from a file. As in the listing (D-18), the
window keeps running after DEATH, any key restarts `GAME` in the core, and the
player types `ZLOAD <name>`. **[SRC]** The cassette is outside RAM: `SAVE` writes
only `DP.BEG` through `MM.END` to tape (`COMMON.ASM:80-86`) and `LOAD` reads it back
(`:102`). So the platform puts stored `.dagram` saves on the core's cassette at start-up
through `Game::insert_cassette_image`, which accepts only DAGRAM 1 images and
places them behind any save made in the session. Nothing is loaded until a typed
`ZLOAD`. **Inferred:** offering earlier saves this way stands in for a tape that
already holds them; it is a platform convenience, not source behaviour, and adds
no input path; recorded as D-20 in §13. Regression: `test_inserted_tape_loads_only_when_typed`. Not run:
an interactive desktop check (no display in this session).
