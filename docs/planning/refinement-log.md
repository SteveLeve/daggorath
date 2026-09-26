# Refinement log

Tracker for `refinement-loop.md`. Newest run last.

## Reported symptoms
Steve adds hand-play observations here. The loop works these first.
- Death animation still runs too fast after e5ac0ac. Check the fade and heart timing against the ENDGAM/death path.

## Open questions

## Answered
- **Q1 (loop 1): death and faint fade speed.** The listing gives one redraw + `SYNC` per step: 17 wizard steps in `WIZIX`, and `RLIGHT` down to -8 in `HUPD30`. Real duration is the CoCo's draw time per step, which the listing cannot give. MAME was not available this run (`/tmp/mame-user` gone after reboot). Options: (a) you re-extract MAME and the loop captures it (C-17); (b) you time it on your CoCo 2 or a video; (c) you pick a feel-right number now as [INF]. Which?
  - **Answer (2026-09-26):** option (a). Next run: re-extract MAME and capture C-17. Rebuild steps are in `docs/prompts/phase-1-rom-captures.md` (Ubuntu `mame 0.264+dfsg.1-1` into `/tmp/mame-user`, LWTOOLS 4.25 into `/tmp/lwtools-4.25`). Measure jiffies per `WIZZES` step and per `HUPD30` step; tune the D-14 delays as [ROM].

## Coverage map
Status: `unreviewed` · `matches` · `gap-fixed` · `gap-open` · `deviation` · `needs-human`

The first run seeds this map. Priority order: COMCRE, CRETUR, PATTK, HUPDAT, SOUNDS,
PINCAN, then the ENDGAM/WINNER path, PTURN, PUPDAT, PGET, PUSE, PCLIMB, PREVEA, PEXAM, HUMAN,
then the rest.

| ASM file:label | Our counterpart | Status | Loop | Note |
|---|---|---|---|---|
| COMCRE.ASM:OFIND |  | unreviewed |  |  |
| COMCRE.ASM:FNDOBJ |  | unreviewed |  |  |
| COMCRE.ASM:CFIND |  | unreviewed |  |  |
| COMCRE.ASM:FNDCEL |  | unreviewed |  |  |
| COMCRE.ASM:CBIRTH |  | unreviewed |  |  |
| COMCRE.ASM:VFIND |  | unreviewed |  |  |
| COMCRE.ASM:CREGEN |  | unreviewed |  |  |
| CRETUR.ASM:CMOVE |  | unreviewed |  |  |
| CRETUR.ASM:SHIELD |  | unreviewed |  |  |
| CRETUR.ASM:STEP |  | unreviewed |  |  |
| CRETUR.ASM:STEPOK |  | unreviewed |  |  |
| CRETUR.ASM:CWALK |  | unreviewed |  |  |
| PATTK.ASM:PATTK |  | unreviewed |  |  |
| PATTK.ASM:ATTACK |  | unreviewed |  |  |
| PATTK.ASM:DAMAGE |  | unreviewed |  |  |
| PATTK.ASM:ENDGAM |  | unreviewed |  |  |
| HUPDAT.ASM:HUPDAX |  | needs-human | 1 | Death/faint fade pacing: listing gives one redraw per SYNC; real speed depends on draw cost. See Q1, C-17. |
| HUPDAT.ASM:HUPD30 |  | needs-human | 1 | Fade-out per-step delay (90 ms) is [INF]; see Q1. |
| HUPDAT.ASM:HUPD40 |  | gap-open | 1 | Wake-up fade-in (HUPD42 raises lighting to OLIGHT) is not drawn; D-14 says no fade-in on waking. |
| SOUNDS.ASM:SNOISE |  | unreviewed |  |  |
| SOUNDS.ASM:SNOUT |  | unreviewed |  |  |
| SOUNDS.ASM:SNWAIT |  | unreviewed |  |  |
| SOUNDS.ASM:CSETUP |  | unreviewed |  |  |
| SOUNDS.ASM:SNENV |  | unreviewed |  |  |
| SOUNDS.ASM:SNENVA |  | unreviewed |  |  |
| SOUNDS.ASM:SETNVA |  | unreviewed |  |  |
| SOUNDS.ASM:SETNVD |  | unreviewed |  |  |
| SOUNDS.ASM:WHOOSH |  | unreviewed |  |  |
| SOUNDS.ASM:CHUCK |  | unreviewed |  |  |
| SOUNDS.ASM:CLANK |  | unreviewed |  |  |
| SOUNDS.ASM:KLANK |  | unreviewed |  |  |
| SOUNDS.ASM:KKLANK |  | unreviewed |  |  |
| SOUNDS.ASM:KLINK |  | unreviewed |  |  |
| SOUNDS.ASM:CLANG |  | unreviewed |  |  |
| SOUNDS.ASM:BANG |  | unreviewed |  |  |
| SOUNDS.ASM:THUD |  | unreviewed |  |  |
| SOUNDS.ASM:KABOOM |  | unreviewed |  |  |
| SOUNDS.ASM:BOOMER |  | unreviewed |  |  |
| SOUNDS.ASM:RATTLE |  | unreviewed |  |  |
| SOUNDS.ASM:GROWL |  | unreviewed |  |  |
| SOUNDS.ASM:GRAWL |  | unreviewed |  |  |
| SOUNDS.ASM:SNARL |  | unreviewed |  |  |
| SOUNDS.ASM:PSSST |  | unreviewed |  |  |
| SOUNDS.ASM:PSSHT |  | unreviewed |  |  |
| SOUNDS.ASM:SQUEAK |  | unreviewed |  |  |
| SOUNDS.ASM:BEOOP |  | unreviewed |  |  |
| SOUNDS.ASM:WHOOP |  | unreviewed |  |  |
| SOUNDS.ASM:PHASER |  | unreviewed |  |  |
| SOUNDS.ASM:GLUGLG |  | unreviewed |  |  |
| SOUNDS.ASM:BDLBDL |  | unreviewed |  |  |
| SOUNDS.ASM:MSQUEK |  | unreviewed |  |  |
| PINCAN.ASM:PINCAN |  | unreviewed |  |  |
| PINCAN.ASM:WINNER |  | unreviewed |  |  |
| PTURN.ASM:PTURN |  | unreviewed |  |  |
| PTURN.ASM:PMOVE |  | unreviewed |  |  |
| PTURN.ASM:PSTEP |  | unreviewed |  |  |
| PUPDAT.ASM:PUPDAX |  | unreviewed |  |  |
| PUPDAT.ASM:PUPSUB |  | unreviewed |  |  |
| PGET.ASM:PGET |  | unreviewed |  |  |
| PGET.ASM:PDROP |  | unreviewed |  |  |
| PGET.ASM:PSTOW |  | unreviewed |  |  |
| PGET.ASM:PPULL |  | unreviewed |  |  |
| PGET.ASM:COMUPD |  | unreviewed |  |  |
| PUSE.ASM:PUSE |  | unreviewed |  |  |
| PCLIMB.ASM:PCLIMB |  | unreviewed |  |  |
| PREVEA.ASM:PREVEA |  | unreviewed |  |  |
| PEXAM.ASM:PEXAM |  | unreviewed |  |  |
| PEXAM.ASM:EXAMIN |  | unreviewed |  |  |
| PEXAM.ASM:PRTOBJ |  | unreviewed |  |  |
| HUMAN.ASM:PLAYER |  | unreviewed |  |  |
| HUMAN.ASM:HUMAN |  | unreviewed |  |  |
| MISC.ASM:WIZIX | src/platform/sdl_app.cpp (death block) | needs-human | 1 | 17 WIZZES steps, each ZFLOP+VCTLST+SYNC; 110 ms/step is [INF]; NOISEF 30 Hz buzz, with NOISEV set to the fade value, not reproduced. |
| remaining .ASM files | | unreviewed | | Split into labels when reached. |

## Run history
| Date | Loop | Targets | Findings | Fixes / commits | Not run |
|---|---|---|---|---|---|
| 2026-09-26 | 1 | Reported: death animation too fast (HUPDAT DEATH/HUPD30, MISC WIZIX) | Pacing unprovable from the listing (per-step draw cost); wake-up fade-in and fade-in buzz not reproduced | none; map seeded, Q1 raised, C-17 added | ROM capture — not run: MAME not extracted |
