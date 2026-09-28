# Phase 5b — An honest power-on-to-WINNER playthrough

Read `CLAUDE.md`, `docs/project-instructions.md`, `docs/archaeology/phase-5/reconciliation.md`
(the "Playthrough harness" section and the 2026-09-27 correction) and
`docs/provenance/ledger.md` first. Work on its own branch from `main` (for example `phase-5b/honest-playthrough`).

## Why this phase exists

`ctest playthrough_power_on_to_winner` is **disabled**. The committed script
(`docs/archaeology/phase-5/traces/power-on-to-winner.script`) was authored by the planner
`src/app/dplan.cpp` against two core bugs, both fixed on 2026-09-27:
- an incanted fire ring lasted 256 swings, where the listing gives VULCAN's 3 charges;
- a spent ring kept 255/255 offense, where the listing makes it a plain gold ring (0/5).

It also relies on harness shortcuts that are not source behaviour (D-12):
- `FUDGE incoming 25`, which quarters creature damage;
- 2290 `FUDGE rest` lines, which set `PDAM` to 63;
- `Game::snapshot()` checkpoints in `.cache/playthrough`.

Re-running `dplan --fudge` now stops with "rings not ready" at the level-2 image.

## Goal

A keystroke script, replayable by `dcli` from power-on, that reaches `WINNER` in
Original Mode:
- **No `FUDGE` lines of any kind.** Creature damage stays at 100%, and healing comes only
  from `HSLOW`, flasks and time.
- **Progress is saved with real `ZSAVE <name>` and resumed with real `ZLOAD <name>`.**
  After a death, the script does what a player would: it recovers the way the original
  allows (see "Death and reload" below), `ZLOAD`s the latest save, and carries on. It
  never starts over from power-on.
- Deterministic: two replays give byte-identical traces, and the ctest is re-enabled with
  the new script and hash.

## Strategy sources

- The original is deterministic. The five maps are fixed, and creature placement follows
  `SECOND` at level entry and the keystroke timing. A route that works once works every
  time for the same script.
- Use published community walkthroughs and strategy guides for *strategy*: route, item
  order, which ring for which wizard, when to build power, flask use, and the image and
  wizard fights. `dplan.cpp` already cites Nemitz, "A Tour of Daggorath". Search for
  others: GameFAQs, CoCo community pages, the Dungeons of Daggorath fan sites.
- Record every guide you rely on in `docs/provenance/ledger.md` as a secondary, strategy-only
  source, with URL, author and date retrieved. Never copy code or data tables from a
  guide or a port. Every mechanic the planner relies on must be checked against this core
  and the listing, not the guide.
- The mechanics now in force are the ones any route must respect:
  - Rings have three charges, then become useless gold rings.
  - A kill adds creature power ÷ 8 to `PPOW` (`PATT42`).
  - Flasks: THEWS adds 1000 power, HALE heals fully, ABYE hurts.
  - Darkness gives a 25% hit gate.
  - Exertion is (weight ÷ 8) + 3 per move, plus the attack cost.
  - D-15 redraw jiffies apply.

## Death and reload

Establish what the listing does after death, and record it:
- `HUPDAT.ASM` `DEATH` ends with `CLR FAINT`, `DEC AUTFLG`, `BRA *`.
- With `AUTFLG` set, `COMMON.ASM` `CLOCK` (around line 490) transfers to `GAME` on any
  keypress.
- The cassette is outside RAM, so a following `ZLOAD` can restore the last save.

The core halts on death and does not model that restart; the desktop fakes it in
`sdl_app.cpp`. If the script is to recover from a death, model the `AUTFLG` restart in the
core first, as source-proven behaviour:
- it gets its own regression test;
- the in-memory cassette (D-11) survives the restart;
- it needs no harness API.

If you decide instead that a death-free script is cleaner, where the planner backtracks
to its last `ZSAVE` internally and the committed script never dies, say why in the
reconciliation. Either way, the committed script contains only keystrokes.

## Work plan

1. Read `dplan.cpp` (about 1900 lines): its phases, `recover()`, the KillImage and
   KillWizard logic, and the checkpoint and fudge code. Summarise what must change.
2. Remove the `--fudge` path, `FUDGE` emission and snapshot checkpoints from the planner.
   Keep `dcli`'s `FUDGE` parsing only if other tests still use it (D-12).
3. Replace the checkpoints with typed `ZSAVE <stage>` at safe points, and with the death
   and reload behaviour above.
4. Rework the strategy for three-charge rings and honest damage: build power before the
   image and the wizard, choose the ring each fight needs, and rest by waiting.
5. Plan, replay twice, and compare hashes. Then re-enable
   `playthrough_power_on_to_winner` in `tests/CMakeLists.txt`.
6. Update `phase-5/reconciliation.md` "Playthrough status" with the new jiffy count,
   saves, deaths if any, and hash. Retire the D-12 rows that no longer apply in
   `clock-and-scheduler.md` §13.
7. Run the `evidence-auditor` agent, and `boundary-checker` if `src/core` changed.
   `make all` must pass.

## Out of scope

- Changing any rule to make the route easier.
- New game modes.
- Planner speed-ups that change what the game sees.

Questions go in `docs/planning/refinement-log.md` under "Open questions".
