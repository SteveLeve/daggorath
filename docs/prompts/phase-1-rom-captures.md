Finish **Phase 1: the remaining ROM captures** for the Dungeons of Daggorath
preservation project, on branch `cursor/phase-1-level-population` (PR #1,
tracking issue #2). Read `CLAUDE.md`, `docs/project-instructions.md`,
`docs/specification/clock-and-scheduler.md`,
`docs/archaeology/phase-0b/traces/README.md` §3,
`docs/archaeology/phase-1/reconciliation.md` §1 and §5,
`docs/provenance/ledger.md` §1 and `tools/rom/README.md` before starting.
Produce captures and recorded evidence, not a plan. This prompt continues
`phase-1-conformance-and-creatures.md` steps 3, 5 and 6; it does not replace
its preservation requirement, which still overrides anything here.

**Where things stand (commit 1e78d74).** Phase 1 steps 1, 2, 4, 5 and 6 are
done, and step 3 is partial. The pinned listing assembles to the catalog
26-3093 image byte for byte. The harness runs on MAME 0.264 `coco2b`: the
owner's CoCo 2B firmware, with the rights basis recorded in the ledger. All five
Phase 0b scripts are captured, deterministic across runs, and their first
divergences are in reconciliation §1. The first divergence is the `INIT` clock
(`0:0:1.0.0` vs `0:0:6.2.5`), because the ROM builds level 0 for 377
interrupts with the clock running; that is issue #13. Do not re-derive any of
this, and do not re-open the firmware question. It is settled.

**Environment.** Check these before calling anything blocked:

```sh
export PATH="/tmp/lwtools-4.25/lwasm:$PATH"
export LD_LIBRARY_PATH=/tmp/mame-user/usr/lib/x86_64-linux-gnu
export DOD_MAME=/tmp/mame-user/usr/games/mame
export DOD_HASHPATH=/tmp/mame-user/usr/share/games/mame/hash
export DOD_FIRMWARE=$HOME/coco-firmware
tools/rom/assemble.sh && tools/rom/run-capture.sh
```

The `/tmp` tools do not survive a reboot. If they are missing, re-extract the
Ubuntu `mame 0.264+dfsg.1-1` package into `/tmp/mame-user` and rebuild
LWTOOLS 4.25 from its tarball. Record that in the ledger if the version
changes. Never copy firmware, the `.ccc`, or anything under `captures/` into
the tree.

**Harness facts you must keep.** `capture.lua` counts one game interrupt per
first `JIFFY` write inside `CLOCK`. It injects keys at the PIA (write tap on
`$FF02`, read tap on `$FF00`), holds SPACE to leave the autoplay demo, and
takes jiffy 0 as the first interrupt after `GAME50` is fetched. It does not
use MAME's frame notifier or `ioport` `set_value`: both were tried and failed,
and the reasons are in `tools/rom/README.md`. Opcode-fetch read taps work on
this build (`GAME50` proves it). Use them for entry points rather than
halting breakpoints.

**Do not loop.** If a capture cannot be done, write "not run: <one-line
reason>" in its reconciliation row, state it once, and move to the next item.
Do not re-check the same obstacle in a later step or restate it in several
documents.

Work in this order.

**1. Scheduler task log.** Add an opcode-fetch tap on `SCHED_JSR` (`C208`,
`JSR [P.TCRTN,U]`). On each fetch, log the interrupt number and the routine
address read from `P.TCRTN,U`, and resolve it to a symbol. Also detect command
dispatch: `LINBUF`/`LINPTR` reset, or a tap on the parser entry. Emit the
events these give you (`TASK run <name>` and `LINE`) in the existing trace
format, so ROM and reference lines can be compared directly. Keep the raw log
separate from the trace. Then:
- replace the D-4 upper bounds in §1 with measured dispatch-to-state-change
  durations for the turn sweep (`T R`, `T A`) and the half-step MOVE;
- capture simultaneous expirations. Arrange a jiffy where jiffy-queue and
  minute-queue tasks expire together, and record the executed order. That is
  the D-1/D-2 measurement.

**2. Issue #13.** Tap the `DGEN90` fetch during the level-0 build and record
`SEED` and `SECOND` at entry and exit. Sample `CMXLND` and the creature control
blocks at scheduler entry, and compare them with `population-entry.txt`
(`cregen 24 24 25 9 25`). Report the result in #13 and reconciliation §1. The
modelling decision (count the build interrupts, derive them, or keep
`SECOND = 1` as a documented deviation in `clock-and-scheduler.md` §13) belongs
to the owner. Present the evidence and a recommendation; do not change the
core, a fixture, or a stored trace in this step.

**3. Level entry, re-entry and the `CREGEN` boundary.** Prefer natural play:
- derive a keystroke route from level 0 to its descent from the fixtures
  (`MAZLND`, vertical features);
- descend at several clock times, return, and dump `MAZLND`, `SEED`, `CMXLND`
  and the control blocks at each `NEWLVL` exit;
- run an idle capture past the five-minute boundary (use `-nothrottle`; raise
  the `-seconds_to_run` backstop for this run only), including a
  leave-and-return.

Level 0 is entered at a fixed clock on the ROM, so `SECOND` = 0, 1, 7, 30, 59
may only be reachable on deeper levels, or by writing `SECOND` from the
harness. If you write RAM, label that capture "harness-modified state", say
exactly what was written and when, and never cite it as unmodified ROM
behaviour.

**4. Sound.** Tap `SNOISE` (`C8CE`) entry and exit, and DAC writes (`PIA$1`,
`$FF20`). Record `SEED` before and after each call (`SNDRND` is separate from
`SEED`), and how many interrupts the foreground is blocked during a sound.
Find a keystroke path that triggers a sound with the least play needed.

**5. Loose ends from step 3.**
- Blocked MOVE is observed at one carried weight (35, so +7). Isolating
  `(POBJWT >> 3) + 3` needs a second weight. Capture it only if a script can
  change the load cheaply; otherwise record "consistent at one weight".
- t3 and t5 cannot be replayed through the keyboard (`POLCAT` delivers at most
  one key per interrupt). Record them as reference-only inputs. If you add a
  buffer-level injection experiment, label it as bypassing `POLCAT`.
- If line-for-line comparison keeps stopping at the known `INIT` clock line,
  add an option to `trace_diff.py` that compares selected event kinds with
  jiffies relative to `INIT`. The literal first divergence must still be
  reported first.

**6. Reconcile and close.**
- Update reconciliation §1 in place, row by row.
- Update `clock-and-scheduler.md` only where a capture changes a label or
  retires D-1, D-2, D-4 or D-5. Leave D-6.
- Promote nothing to ROM-observed without the capture that shows it.
- Update issue #2's checkboxes, PR #1's description, and #13.

**Completion gate.** For every §1 row, give either a first divergence with its
jiffy and both values, "none", or "not run" with its reason. Run `make all` and
report the actual output: ctest counts, `make verify`, and any first
divergence. Confirm `git status --porcelain` shows no ROM, firmware or capture
file. State plainly which claims are now ROM-observed and which remain
source-derived. Mark PR #1 ready only when the gate holds and the owner has
decided #13.

**Not in scope.** `CMOVE` movement or retiring D-6, attacks and the `PATT40`
decrement, combat, items, magic, rendering, audio playback, save/load, SDL,
mobile UI, and any change to core behaviour made to "fix" a divergence. A
divergence gets an issue and a recorded decision first.
