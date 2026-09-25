---
name: phase-check
description: Confirm the working tree stays inside the open phase and passes the full build, test, trace and verify pipeline.
disable-model-invocation: true
---

1. Read `README.md` to find the open phase and its prompt under `docs/prompts/`.
   Read the phase sequence in `docs/project-instructions.md`.
2. Run `git status --porcelain` and `git diff HEAD --stat`. For each changed
   area, decide whether it belongs to the open phase. Flag work that jumps ahead of
   the charter sequence, such as combat, creatures or mobile UI before their phase.
3. Run `make all` (fixtures, build, test, traces, verify). Report any failure
   with its output. Don't paper over it.
4. If traces changed, run `git diff --stat docs/archaeology/phase-0b/traces` and
   say whether each change is expected.
5. Delegate to the `evidence-auditor` and `boundary-checker` agents, then merge
   their findings.
6. Summarise: in scope / out of scope, pipeline status, and the blocking findings.
