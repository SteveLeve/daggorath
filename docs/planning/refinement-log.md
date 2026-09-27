# Refinement log

Tracker for `refinement-loop.md`. Newest run last.

## Reported symptoms
Steve adds hand-play observations here. The loop works these first.
- ~~Death animation too fast~~ fixed by C-17 (2026-09-26). Hand-play check pending.

## Open questions


## Answered
- **Q3 (loop 6): spent rings, and the playthrough.** Two core bugs, both [SRC] and confirmed in the other port's code (reference only). (a) `OCBFIL` writes `P.OCXXX` only on an `XXXTAB` match (OBIRTH.ASM:73-85), but the core zeroed it. So an incanted fire ring started with 0 charges and wrapped to 256 swings instead of VULCAN's 3. (b) When the last charge goes, `PATTK` sends the ring through `PREV00`, and `OCBFIL` gives it the gold ring's 0/5 offense and clears reveal. The core kept 255/255, so a spent ring was a permanent auto-hit weapon. The fix, with tests, is on branch `refinement/ring-charges`, and the evidence audit found no problem with the listing reading. The Phase 5 power-on-to-WINNER playthrough depends on the old bugs: re-planning with `dplan --fudge` stops with "rings not ready" at the level-2 image with the fire ring spent. Merging needs the planner's strategy reworked (power build-up plus sword fights, or a ring per wizard), and the reconciliation must say why the old script and hash are superseded. OK to merge the fix and take on the planner rework, or disable the playthrough test with a recorded reason until then?
  - **Answer (2026-09-27):** merge the fix and disable the playthrough test; the planner rework follows separately (done 2026-09-27; phase-5 reconciliation).
- **Q2 (loop 5): charge the PUPDAT SYNC in the core?** [SRC] `LUKNEW` (COMPLR.ASM:53-63) redraws via `PUPDAT` whenever a creature has moved into view range (`CWALK` sets `NEWLUK`) or the map is up. `CMOVE` also calls `PUPDAT` after each pickup and when it ends on the player's cell. `PUPDAX` then `SYNC`s unless fainted, so the foreground loses at least 1 jiffy each time. The core had `LUKNEW` as a stub and charged none of this. Branch `refinement/pupdat-sync` (ef68ecd) adds the charge using the same next-pass approximation as D-4a, and saves `NEWLUK`. All tests pass except the two Phase 3 fight traces, which gain a `PUPDAT`+`SYNC` jiffy at 4 points each. Merge it and regenerate those two baselines, with the reason recorded in phase-3 reconciliation? Or wait for ROM capture C-19 first?
  - **Answer (2026-09-27):** merge and regenerate the phase-3 baselines (done 2026-09-27; phase-3 reconciliation).
- **Q1 (loop 1): death and faint fade speed.** The listing gives one redraw + `SYNC` per step: 17 wizard steps in `WIZIX`, and `RLIGHT` down to -8 in `HUPD30`. Real duration is the CoCo's draw time per step, which the listing cannot give. MAME was not available this run (`/tmp/mame-user` gone after reboot). Options: (a) you re-extract MAME and the loop captures it (C-17); (b) you time it on your CoCo 2 or a video; (c) you pick a feel-right number now as [INF]. Which?
  - **Answer (2026-09-26):** option (a). Next run: re-extract MAME and capture C-17. MAME and lwasm were re-extracted to `~/coco-tools` on 2026-09-26; run `. tools/rom/env.sh` before capturing. Measure jiffies per `WIZZES` step and per `HUPD30` step; tune the D-14 delays as [ROM].

## Coverage map
Status: `unreviewed` · `matches` · `gap-fixed` · `gap-open` · `deviation` · `needs-human`

The first run seeds this map. Priority order: COMCRE, CRETUR, PATTK, HUPDAT, SOUNDS,
PINCAN, then the ENDGAM/WINNER path, PTURN, PUPDAT, PGET, PUSE, PCLIMB, PREVEA, PEXAM, HUMAN,
then the rest.

| ASM file:label | Our counterpart | Status | Loop | Note |
|---|---|---|---|---|
| COMCRE.ASM:OFIND | creature_move.cpp find_unowned | matches | 4 | Level, row/col, owner==0; callers start with OFINDF cleared. |
| COMCRE.ASM:FNDOBJ | (inlined in OFIND/attach_objects scans) | matches | 4 | Linear scan of the OCBs on LEVEL. |
| COMCRE.ASM:CFIND | population.cpp occupied, creature_move.cpp cell_occupied | matches | 4 | Dead CCBs are skipped by the P.CCUSE test. |
| COMCRE.ASM:FNDCEL | population.cpp birth_creatures | matches | 4 | RNDCEL column then row, retry on $FF. Does not exclude the player's cell, in the original too. |
| COMCRE.ASM:CBIRTH | population.cpp birth_creatures | matches | 4 | First free CCB, CDB copy, FNDCEL+CFIND retry; C-02 [ROM]. |
| COMCRE.ASM:VFIND | population.cpp vfind | matches | 4 | Miss returns -126 on the ROM and -1 in the core; both callers (PCLIMB, VIEWER) test only BMI. |
| COMCRE.ASM:CREGEN | game.cpp task_cregen, population.cpp cregen_increment | matches | 2 | 8-bit sum over 12 types, BHS 32, RANDOM&7+2, 5-minute reschedule. |
| CRETUR.ASM:CMOVE | creature_move.cpp cmove, game.cpp task_cmove | gap-fixed | 4 | Logic matches: frozen before dead, scorpion/wizard skip pickup, pickup then CMOV90, player-cell attack with SHIELD, line-of-sight walk, MOVTAB preference, back-out. **Gap [SRC]:** CMOVE calls PUPDAT after every pickup (CRETUR CMOV10) and at CMOV90 when on the player's cell. PUPDAX redraws and waits on SYNC unless fainted (PUPDAT.ASM:3-8), which costs the foreground at least 1 jiffy. The core only emits a PUPDAT event and charges no time. Fixing it touches scheduler timing and traces, so measure first: C-19. |
| CRETUR.ASM:SHIELD | creature_move.cpp apply_shield | matches | 4 | Class K.SHIE; the lower 16-bit (MGD<<8 or PHD) pair wins, BHS keeps the current one; $8080 default stored every attack. |
| CRETUR.ASM:STEP |  | unreviewed |  |  |
| CRETUR.ASM:STEPOK | creature_move.cpp step_ok | matches | 4 | Border then $FF solid-wall test; used by both the line-of-sight walk and CWALK. |
| CRETUR.ASM:CWALK | creature_move.cpp cwalk | matches | 5 | STEPOK+CFIND, big<=8/little<=2 window, RANDOM bit 0, volume ~(big*31); sets NEWLUK (see Q2). |
| PATTK.ASM:PATTK | game.cpp cmd_attack | gap-fixed | 6 | Exertion (9-bit sum /8), SNDOBJ, ring auto-hit, darkness 25% gate, KLK2, !!! match. Spent ring was missing PREV00/OCBFIL, fixed on refinement/ring-charges (Q3). Kill path lacks the PUPDAT SYNC (Q2 class). |
| PATTK.ASM:ATTACK | combat attack_hits | matches | 6 | Fixture-backed (combat fixtures). |
| PATTK.ASM:DAMAGE | combat apply_damage | matches | 6 | SCAL16 magic then physical; fixture-backed. |
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
| PINCAN.ASM:PINCAN | game.cpp cmd_incant, incant_hand | matches | 8 | ADJTAB + FULFLG, K.RING, P.OCXXX+1 match, OCBFIL, A$RING, CLR +1, T.RN15 goes to WINNER. The ROM tries the right hand even after the left incants (BSR, then fall-through), while the core stops after the left. That only differs if both hands hold a ring answering the same word [INF]; not changed. |
| PINCAN.ASM:WINNER | game.cpp incant_hand WINNER | matches | 8 | WIZ2 fade-in, two OUTSTI, BRA *; test_winner. |
| PTURN.ASM:PTURN |  | unreviewed |  |  |
| PTURN.ASM:PMOVE | game.cpp cmd_move; sdl_app half-step/wipe | deviation + gap-open | 8 | Core: parse, PSTEP, PMOV90 exertion ((POBJWT asr 3)+3) on every path, SYNC; the half-step PUPDAT timing is D-4a [ROM]. **Presentation gap:** a forward or back MOVE draws the half-step view (HLFSTP/BAKSTP PUPDAT) before PSTEP, even when PSTEP then THUDs into a wall. The desktop shows a half-step only when the position changes, so a wall bump shows no lunge. It needs the MoveAnimation block to carry the direction (event schema change plus a test). A blocked sidestep correctly shows no wipe (BNE PMOV90). |
| PTURN.ASM:PSTEP | game.cpp step_player | matches | 8 | STEPOK; on failure ISOUND A$THUD and the position is kept; PUPSUB redraws the backplane. |
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
| 2026-09-26 | 4 | COMCRE (OFIND, FNDOBJ, CFIND, FNDCEL, CBIRTH, VFIND), CRETUR (CMOVE, SHIELD, STEPOK) | All match except the CMOVE PUPDAT SYNC time, which is not charged | none; CMOVE gap-open, C-19 added | Fix — deferred until C-19 measures the cost |
| 2026-09-26 | 5 | CWALK, COMPLR LUKNEW | CWALK matches; LUKNEW was a stub, so neither LUKNEW nor CMOVE charged the PUPDAT SYNC | Fix on branch refinement/pupdat-sync (ef68ecd), not merged; Q2 raised | Baseline regeneration — not run: needs Steve's call (Q2) |
| 2026-09-26 | 6 | PATTK, ATTACK, DAMAGE, OCBFIL | Spent rings kept 255/255 offense; OCBFIL zeroed ring charges (256 swings instead of 3) | Fix on branch refinement/ring-charges (44d428c + labels); Q3 raised | Playthrough re-plan — not run: planner blocked, needs strategy rework (Q3) |
| 2026-09-27 | 7 | Q2/Q3 follow-through | Merged both fixes; phase-3 and phase-0b baselines regenerated with reasons recorded; D-15 added; playthrough disabled; two regression tests; audited | this commit | No regression test yet for the CMOV90 NEWLUK clear or the two-SYNC pickup on the player's cell |
| 2026-09-27 | 8 | PINCAN, WINNER, PMOVE, PSTEP | All match; PMOVE's wall-bump half-step view is missing in the desktop (gap-open) | log only | Wall-bump fix — not run: needs an event-schema change, next run |
