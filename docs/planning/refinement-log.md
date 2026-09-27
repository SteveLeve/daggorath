# Refinement log

Tracker for `refinement-loop.md`. Newest run last.

## Reported symptoms
Steve adds hand-play observations here. The loop works these first.
- ~~Death animation too fast~~ fixed by C-17 (2026-09-26). Hand-play check pending.

## Open questions

## Answered
- **Q1 (loop 1): death and faint fade speed.** The listing gives one redraw + `SYNC` per step: 17 wizard steps in `WIZIX`, and `RLIGHT` down to -8 in `HUPD30`. Real duration is the CoCo's draw time per step, which the listing cannot give. MAME was not available this run (`/tmp/mame-user` gone after reboot). Options: (a) you re-extract MAME and the loop captures it (C-17); (b) you time it on your CoCo 2 or a video; (c) you pick a feel-right number now as [INF]. Which?
  - **Answer (2026-09-26):** option (a). Next run: re-extract MAME and capture C-17. MAME and lwasm were re-extracted to `~/coco-tools` on 2026-09-26; run `. tools/rom/env.sh` before capturing. Measure jiffies per `WIZZES` step and per `HUPD30` step; tune the D-14 delays as [ROM].

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
| COMCRE.ASM:CREGEN | game.cpp task_cregen, population.cpp cregen_increment | matches | 2 | 8-bit sum over 12 types, BHS 32, RANDOM&7+2, 5-minute reschedule. |
| CRETUR.ASM:CMOVE |  | unreviewed |  |  |
| CRETUR.ASM:SHIELD |  | unreviewed |  |  |
| CRETUR.ASM:STEP |  | unreviewed |  |  |
| CRETUR.ASM:STEPOK |  | unreviewed |  |  |
| CRETUR.ASM:CWALK |  | unreviewed |  |  |
| PATTK.ASM:PATTK |  | unreviewed |  |  |
| PATTK.ASM:ATTACK |  | unreviewed |  |  |
| PATTK.ASM:DAMAGE |  | unreviewed |  |  |
| PATTK.ASM:ENDGAM |  | unreviewed |  |  |
| HUPDAT.ASM:HUPDAX |  | gap-fixed | 1 | Fade pacing set from ROM capture C-17 (9e8d66c, ab91dc7). |
| HUPDAT.ASM:HUPD30 |  | gap-fixed | 1 | 5 jiffies per step [ROM], C-17. |
| HUPDAT.ASM:HUPD40 |  | gap-fixed | 3 | Wake-up fade-in climbs to the saved OLIGHT [SRC], 5 jiffies per step [ROM] C-18; the lighting drift of +1 is washed out by PUPSUB [SRC]. |
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
| PUPDAT.ASM:PUPDAX | game.cpp PUPDAX redraw | matches | 2 | HUPD32 clears KBDHDR/KBDTAL; the core leaves the buffer but PLAYER discards every char while fainted (HUMAN.ASM:21), and CLK50 stops polling, so the effect is the same [INF]. |
| PUPDAT.ASM:PUPSUB | game.cpp refresh_light | matches | 2 | PRLITE is non-zero only after ENDGAM (PATTK:158), when there is no torch; the core writes 7/$13 directly and refresh_light cannot run then. |
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
| MISC.ASM:WIZIX | src/platform/sdl_app.cpp (death block) | gap-fixed | 1 | 18 jiffies per step [ROM], C-17; still open: NOISEF buzz. 17 WIZZES steps, each ZFLOP+VCTLST+SYNC; 110 ms/step is [INF]; NOISEF 30 Hz buzz, with NOISEV set to the fade value, not reproduced. |
| COMPLR.ASM:HSLOW | game.cpp task_hslow | matches | 3 | ASRD6 of -PDAM, signed BGT floor at 0, reschedule HEARTR on Q.JIF; the core's delay-0 guard needs D > P, which is already death [INF]. C-18 shows healing continuing while fainted. |
| remaining .ASM files | | unreviewed | | Split into labels when reached. |

## Run history
| Date | Loop | Targets | Findings | Fixes / commits | Not run |
|---|---|---|---|---|---|
| 2026-09-26 | 1 | Reported: death animation too fast (HUPDAT DEATH/HUPD30, MISC WIZIX) | Pacing unprovable from the listing (per-step draw cost); wake-up fade-in and fade-in buzz not reproduced | none; map seeded, Q1 raised, C-17 added | ROM capture — not run: MAME not extracted |
| 2026-09-26 | 1b (manual) | C-17 capture | Wizard fade ~4.8 s on the ROM vs our 1.9 s; blank gap 33 ms vs our 400 ms | 9e8d66c, ab91dc7; audited | — |
| 2026-09-26 | 2 | HUPD40/HUPD42, PUPDAX, PUPSUB, CREGEN | Wake-up fade missing; desktop fade-out drew one step early; PUPSUB, keyboard flush and CREGEN match | Wake fade-in to saved OLIGHT, fade-out order fixed; C-18 added; audited | Revival capture — not run: C-18 next |
| 2026-09-26 | 3 | C-18 revival capture, HSLOW | Wake fade 5 jiffies/step confirms 83 ms; the 400 ms faint hold was invented (ROM: 1 jiffy); HSLOW matches | Faint hold 33 ms, wake pacing [ROM]; audited | — |
