# Phase 2 reconciliation — creature movement

Dated 2026-09-25. Prompt: [`../../prompts/phase-2-creature-movement.md`](../../prompts/phase-2-creature-movement.md).
Issue #3.

## 1. What changed

`CBIRTH` queues `CMOVE` on `Q.TEN` with the movement delay from `CDBTAB`.
D-6 is retired. `CMOVE` runs the listing's priority order through the step
that would call `ATTACK`. That call is `DEFER creature-attack <slot>` (D-7,
Phase 3). ADR-0002 is accepted: one bounded lap per jiffy (option 3).
Countdown lists follow `QUEADD` order.

No claim in this phase is ROM-observed. C-09, C-10, and C-11 are still
uncaptured. The 10-minute file is core output.

## 2. Trace differences against Phase 0b / Phase 1

`t1`–`t5` were regenerated. Fixture hashes did not change (`make verify`:
17 fixtures, 0 problems). The first divergence in `t2-forward-corridor.trace`
is the `HSLOW` / `PLAYER` tie at jiffy 47:

- previous: `QUEUE ready PLAYER`, then `TASK run PLAYER`, then `TASK run HSLOW`
- current: `QUEUE ready HSLOW`, then `QUEUE ready PLAYER`, then `TASK run HSLOW`, then `TASK run PLAYER`

Cause: `Q.JIF` is now a `QUEADD` list. `PLAYER` expires every jiffy and is
appended behind `HSLOW`. This is the listing's link order, not D-6. The same
reorder appears in the other four traces at the first shared jiffy-queue
expiry. Creature lines begin later, at jiffy 85 in `t2` (`QUEUE ready CMOVE-6`,
the first viper; movement delay 15). Those lines are retired D-6.

Cross-queue interrupt order is unchanged: the jiffy queue is still scanned
before a tenth or minute queue. The Phase 1 sample `PLAYER`, `LUKNEW`,
`BURNER` is that cross-queue order.

## 3. Labels

Source-proven, with listing sites in [`../../specification/creatures.md`](../../specification/creatures.md) §6:
frozen-before-dead, pickup exclusions, one object, alignment directions,
`MOVTAB` and the 64/256 side-first case, back-off, `STEPOK`, `Q.TEN` delays,
and the attack-delay selection when the cell is the player's.

Inferred: a `Q.SCD` task runs at most once per jiffy (D-1, D-2); queue 0 is
never scanned, so a dead creature's task is dropped.

Unresolved: how many source laps fit in one jiffy; `enter_level` does not
call `SYSTCB`, so a re-entry can leave `LUKNEW` ahead of the new `CMOVE`
tasks; hand contents after the player picks something up (hands start empty).

## 4. Gate

`make all` on 2026-09-25 exited 0.

- `ctest`: 2 tests, 0 failures.
- Conformance: `PASS: 659 checks, 0 failures`.
- `make verify`: `OK: 17 fixtures, 0 problems`.
- No fixture hash changed. `creatures.json` already carried movement and
  attack delays.

## 5. Next slice

Phase 3: the attack branch D-7 names, including `ATTACK` / `DAMAGE` and the
matrix decrement. Do not treat the deferred trace line as a substitute for
that effect.
