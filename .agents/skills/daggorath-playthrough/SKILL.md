---
name: daggorath-playthrough
description: Plan, implement, or verify Daggorath Phase 5b's power-on-to-WINNER keystroke replay, including source-backed death recovery and real cassette save/load.
---

Use the repository root as the base for paths below, including when invoked from
another directory. Read `AGENTS.md`, `CLAUDE.md`, then the authoritative documents
and active prompt they identify. The goal and acceptance contract live in
`docs/prompts/phase-5b-honest-playthrough.md`; do not maintain a second copy here.

## Work isolation and sequence

Confirm `git worktree list`, branch, HEAD, and status before editing. Work in the
dedicated Phase 5b worktree on `refinement/phase-5b-honest-playthrough`. Preserve
other worktrees and existing changes. Read `docs/planning/phase-5b-handoff.md` for preparation status and
continuation steps. Validate scaffolding before gameplay edits; reuse recorded
validation unless the relevant scaffold changed or the host no longer loads it.

Inspect `src/app/dplan.cpp`, the core save/load and scheduler paths, and existing
regressions before choosing changes. Keep implementation slices independently
reviewable: recovery, planner, replay acceptance, then evidence closure.

## Planning tools and evidence

Full read-only game-state inspection, prediction, route search, simulations,
parallel experiments, and strategy research are allowed. Hidden information is
not a violation. Scratch copies/snapshots of reachable, unmodified states are
allowed for faithful exploratory simulations. Keep them separate from the
candidate run; they cannot supply its state or replace its recorded recovery.
Do not introduce snapshot restoration into candidate checkpoint/recovery code;
all candidate progress and recovery must be reproducible through recorded keys.

Trace required mechanics to the pinned listing and existing tests. Treat community
guides as strategy suggestions; record relied-on sources in the provenance ledger.
A winning core replay does not establish ROM conformance. Record uncertain timing
without inventing it or blocking unrelated, source-backed progress.

## Candidate handling

Use a distinct temporary output for each candidate and retain diagnostics for
failed attempts. Do not overwrite the committed script or hash until the candidate
passes independent acceptance. Within a candidate, retain deaths and recovery
inputs. On a repeated failure after reload, change the strategy or key timing;
do not retry identical deterministic input indefinitely. Record bounds and the
last successful save in failure reports.

Keep planner bookkeeping separate from game state. Resume after a successful
ZSAVE event, not merely after typing the command. Use distinct uppercase alphabetic
save names of at most eight characters. On process interruption, reconstruct the
candidate by replaying its recorded prefix; do not import stale snapshot caches.

## Review and closure

Delegate an independent read-only review to `evidence-auditor` for behaviour,
fixture, specification, or provenance changes. Use `boundary-checker` when core
or boundaries change and `playthrough-reviewer` for planner or acceptance changes.
Both evidence and playthrough reviews are required for phase closure. Their
shared instructions are in `.claude/agents/`; Codex adapters are
in `.codex/agents/`. If a host cannot discover a role, explicitly supply its shared
instructions to an available subagent and report that native discovery was not
verified. Do not claim configured agents or checks ran without execution evidence.

Run the prompt's acceptance checks and the README's applicable verification
commands. `make all` rewrites generated artifacts: record justified baseline
changes before regeneration and inspect the resulting diff. Never regenerate an
expected result just to make a failure pass. Report complete, failed, and not-run
checks separately; keep the phase open until all acceptance requirements pass.
