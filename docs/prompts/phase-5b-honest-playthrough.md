# Phase 5b — An honest power-on-to-WINNER playthrough

Read `CLAUDE.md`, `docs/project-instructions.md`, `docs/archaeology/phase-5/reconciliation.md`
(the "Playthrough harness" section and the 2026-09-27 correction) and
`docs/provenance/ledger.md` first. Work in the dedicated Phase 5b worktree on
`refinement/phase-5b-honest-playthrough`. Confirm branch, HEAD, and clean/owned
changes before editing; do not modify the original checkout or other worktrees.
Use the shared `daggorath-playthrough` skill and validate its review-agent
scaffolding before changing gameplay. Read
[`../planning/phase-5b-handoff.md`](../planning/phase-5b-handoff.md) for the
preparation status and continuation steps; this prompt owns acceptance criteria.

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
  does not discard its input history or abandon saved progress. The source
  `GAME` restart after death is required, not prohibited. The final script must
  demonstrate at least one death, keypress restart, and successful reload.
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
  - The incanted attack rings have three charges, then become plain gold rings
    with 0/5 offense; verify the relevant ring transitions against the listing.
  - WIZ1's physical defense is zero, but that disables only the Elvish sword's
    physical channel. Its magic offense is 64; at player power 10660, the
    source-derived magic channel deals 249 damage. This does not establish a
    safe attack/recovery cycle.
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
`sdl_app.cpp`. Model the source-proven `AUTFLG` restart control flow in the
core first:
- it gets its own regression test;
- the in-memory cassette (D-11) survives the restart;
- it needs no harness API.

The restart control flow is **source-proven** at the labels above. Its exact
elapsed timing is not established by those labels alone. Record reset/preserved
state, treatment of the triggering key, and any timing inference before coding.
Distinguish foreground halt from interrupt processing. Preserve the cassette,
future timestamped input, and monotonic replay timestamps through restart and
ZLOAD; game clock counters can reset or restore independently. Do not implement
recovery as an external replacement Game or a harness-only restore call.

This phase establishes headless core/dcli recovery. Check desktop compatibility,
but defer replacing its existing death menu and filesystem load shortcut; record
that distinction in reconciliation rather than claiming desktop recovery parity.

## Planner freedom and checkpoints

Full read-only inspection of hidden core state is allowed. Use prediction,
route search, simulations, parallel experiments, debugging, and any available
research tools that simplify the task. Honesty constrains game rules and the
replayed candidate's actions, not the planner's knowledge or human playability.
Separate exploratory simulations from the candidate execution. Scratch copies
and snapshots of reachable, unmodified game states are allowed for faithful
what-if simulations and search. They must not supply state to the final candidate
or substitute for its recorded save/recovery input. Do not add
snapshot restoration to its checkpoint/recovery path or use game-state edits
to make the candidate succeed. Snapshot APIs with unrelated consumers need not
be removed globally.

Candidate actions are recorded timestamped keystrokes; waiting advances every
intervening jiffy. Use unique uppercase alphabetic save names of at most eight
characters. Confirm successful ZSAVE events before accepting checkpoints. Keep
planner bookkeeping outside simulation state. For a process restart, replay the
recorded candidate prefix to reconstruct its cassette and state; do not resume
from old `.cache/playthrough` snapshots.

After a death and reload, the same restored simulation state and relative input
timing reproduce the same failure; changing the absolute trace offset alone is
not a new strategy.
Change route, equipment, or timing; bound attempts and total simulated jiffies,
and report the exhausted bound, last successful save, and failure evidence.
Retain deaths and recovery input within the final candidate. Exploratory runs
that are not selected for the candidate may be discarded.

## Replay acceptance

Generate candidates in separate temporary/build paths. Publish a replacement
script and baseline only after independent verification succeeds. The verifier
must fail on missing output, malformed script/trace, process failure, timeout,
forbidden harness directives, or incomplete progress.

Run two fresh dcli processes with default Original Mode startup: no SECOND/level,
frozen-creature, damage, RAM, or snapshot overrides. Require actual parsed WINNER
events, successful named saves, and at least one ordered death, core restart,
and successful ZLOAD of the latest completed save. Check every death in the
candidate has a recovery sequence. Candidate qualification requires byte-identical
full traces and computes their proposed SHA-256. After that candidate is accepted
and its hash recorded, regression runs must match the recorded hash as well as
each other. Do not require a new candidate to match the superseded baseline or
accept an arbitrary occurrence of the word WINNER.
Provide negative verifier tests for bypasses, missing recovery/victory, mismatched
traces and hashes, and failed replay processes.

Choose replay length from the completed route and timeout from measured runtime
with margin. Record both. Two matching core traces prove deterministic core
replay under documented deviations, not ROM conformance. Record the reason for
replacing the old baseline before updating the hash; never invent expected data.

## Work plan

1. Read `dplan.cpp` (about 1900 lines): its phases, `recover()`, the KillImage and
   KillWizard logic, and the checkpoint and fudge code. Summarise what must change.
2. Remove the `--fudge` path, `FUDGE` emission and snapshot checkpoints from the planner.
   Keep `dcli`'s `FUDGE` parsing only if other tests still use it (D-12).
3. Replace the checkpoints with typed `ZSAVE <stage>` at safe points, and with the death
   and reload behaviour above.
4. Rework the strategy for three-charge rings and honest damage: build power before the
   image and the wizard, choose the ring each fight needs, and rest by waiting.
5. Generate and independently verify a candidate under Replay acceptance. Then re-enable
   `playthrough_power_on_to_winner` in `tests/CMakeLists.txt`.
6. Update `phase-5/reconciliation.md` "Playthrough status" with the new jiffy count,
   saves, demonstrated deaths/recoveries, and hash. Mark this playthrough independent of D-12 in
   `clock-and-scheduler.md` §13; retain documentation for any remaining harness
   APIs or tests. Preserve the superseded baseline history.
7. Run `evidence-auditor` and `playthrough-reviewer` for phase closure. Also run
   `boundary-checker` if core or module boundaries changed. Run the README
   verification commands;
   `make all` must pass for phase completion. It regenerates evidence, so
   record reasons first and inspect generated diffs rather than accepting them
   blindly. Keep the phase open on incomplete or failed acceptance.

## Out of scope

- Changing any rule to make the route easier.
- New game modes.
- Planner speed-ups that change what the game sees.

Questions go in `docs/planning/refinement-log.md` under "Open questions".
