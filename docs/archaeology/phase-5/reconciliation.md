# Phase 5 reconciliation

Labels: **[SRC]** read from the pinned listing; **[INF]** inferred; **[OPEN]** unresolved. Nothing here is ROM-observed.

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
| D-12 `FUDGE incoming` / `FUDGE rest` | **Not source behaviour.** Harness API only (ADR-0007). Default `Game()` stays at 100% incoming damage. `FUDGE incoming 25` multiplies creature-to-player damage by 1/4. `FUDGE rest` writes `PDAM = 63`, a harness value that skips waiting out recovery (the HSLOW stall at 63 was a core bug, corrected 2026-09-26; see phase-3 reconciliation). Player hits are not scaled. Checkpoints under `.cache/playthrough/` restore `snapshot()` and are not a game command. |
| D-1, D-2 lap model | ADR-0002, Track R |
| D-3 `HSLOW` zero-countdown clamp | Track R |
| D-4 animation and sound durations | Phase 6 (listing-derived part), Track R (measured part) |
| D-5 trace counter sampling | Permanent, trace format only |

## Playthrough harness (not source behaviour)

A closed-loop planner (`src/app/dplan.cpp`) records a power-on keystroke script.
A pure power-on run can take hours of HSLOW recovery and dies to a single
scorpion sting at Original Mode incoming damage. Two harness mechanisms exist
so a committed script can still be replayed by `dcli` / ctest:

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

## Open

| Item | Label |
|---|---|
| `OCBPTR` rewind in the ring riddle | [OPEN] The core has no object-allocation pointer, and nothing allocates an object after the riddle. |
| Drawing the death and winner screens | Phase 6 |

## Playthrough status

Authored by `src/app/dplan.cpp`. Committed script:
`docs/archaeology/phase-5/traces/power-on-to-winner.script`.
`ctest -R playthrough_power_on_to_winner` runs `dcli` twice; both traces emit
`WINNER` and are byte-identical.

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
See the scheduler specification's Phase 5b restart section and D-16 for inferred
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
