# Phase 6 reconciliation

`dcli --present` prints a logical draw list. `SNOISE` does not consume `SEED`.

The draw list uses the `NORSCL` bytes on a facing ray. The ray stops when the next cell byte is `$FF`. It does not read wall or door edges, so an open corridor is not a sight blocker. `ahead[0]` is stored and not read. ADR-0004 stays Proposed: there is no `CoreEvent` stream. `VIEWER.ASM` and `SOUNDS.ASM` are cited by the rendering spec and are not rows in the provenance ledger. Sound-loop durations inside `SOUNDS.ASM` remain unresolved (D-4). Screenshot and audio captures for fixed states are not in the capture backlog as completed rows. Later commits on the phase 7 branch decode some `VCTLST` lists; this phase's contract did not.
