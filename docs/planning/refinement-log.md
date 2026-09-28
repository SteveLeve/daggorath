# Refinement log

Tracker for `refinement-loop.md`. Newest run last.

## Working branch
`refinement/discovery-2`, from `main` after PR #23 (runs 17–22) was merged on 2026-09-27. Earlier: PR #21 (runs 1–16). The playthrough rework is `docs/prompts/phase-5b-honest-playthrough.md`.

## Reported symptoms
Steve adds hand-play observations here. The loop works these first.
- ~~Death animation too fast~~ fixed by C-17 (2026-09-26). Hand-play check pending.

## Open questions


## Answered
- **Q6 (2026-09-27, run 22): charge the level-build time on CLIMB?** [ROM] `descend-early`: `C D` is read at isr 1775, the level-1 spin reads `SECOND` at isr 2103 (41), `NEWLVL` exits at 2121 and `PLAYER` resumes at 2130. `C U` at 2280, spin at 2642 (`SECOND` 50). So the foreground is blocked about 330-360 interrupts while `PREPAR` shows "PREPARE!" and `DGNGEN` carves. The core builds instantly with `SECOND` 35, so level 1's creatures land elsewhere (ROM `0:5@28,1 1:5@20,31`, core `0:5@9,13 1:5@4,10`) and the return to level 0 differs too. Carving should be constant per level (fixed seed per level); the spin costs about 0.1 interrupt per draw. Proposal: block the foreground for a measured per-level pre-spin count, read `SECOND` then, and show PREPARE! in the window meanwhile; levels 0 and 1 are measured, levels 2-4 need capture C-22. This moves every climb baseline and the playthrough. Approve, and should C-22 be captured first? **Answer (Steve, 2026-09-27):** accepted; try it and see how it lands, capture first. Done in run 26: C-22 captured, timed NEWLVL applied; descend-early's level 1 and level 0 now match the ROM's creatures.
- **Q5 (2026-09-27, run 17): reverse-video band in a1df32c.** The listing inverts only the status line: NEWLVL.ASM:83-90 NLVL50 sets VDGINV and the P.TXINV flags of TXTPRI and TXTEXA to -(LEVEL&1), then stores the complement to TXTSTS (defaults COMDAT.ASM:96-106: TXTEXA 0, TXTSTS -1, TXTPRI 0). a1df32c inverts the command and message lines too, and the whole screen does not flip on odd levels. There is also no spec text or §13 entry for it. Options: (a) invert only the status line and model the odd-level VDGINV flip, or (b) keep the current look as a recorded deviation. Which? **Answer (Steve):** the status line is always reversed and flips with the level. Done in the polarity commit. The odd-level inversion moved to dag::apply_vdginv with a test in run 18.
- **Q4 (loop 11): charge the GAME50 start-up INIVU?** `INIVU` now runs its whole `INIVUX`/`PLOOK` body: CLRPRI, viewer mode and the D-15 `PUPDAT` charge. The start-up call in `GAME50` is left uncharged, because the capture harness defines jiffy 0 as the first interrupt after `GAME50` is fetched. Charging it would shift every baseline by one jiffy at the start, and the alignment is already [INF]. Leave it, or charge it and regenerate every baseline?
  - **Answer (2026-09-27):** deferred to a start-up alignment task (capture C-21). The ROM's first PLAYER turn comes about 13 jiffies after the core's (t1), so one jiffy here cannot be checked alone. Take it up with the opening-timing work and regenerate the baselines once.
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
| CRETUR.ASM:STEP | creature_move.cpp step_ok | matches | 13 | STPTAB offsets N/E/S/W, then MAP32; reviewed with STEPOK in run 4. |
| CRETUR.ASM:STEPOK | creature_move.cpp step_ok | matches | 4 | Border then $FF solid-wall test; used by both the line-of-sight walk and CWALK. |
| CRETUR.ASM:CWALK | creature_move.cpp cwalk | matches | 5 | STEPOK+CFIND, big<=8/little<=2 window, RANDOM bit 0, volume ~(big*31); sets NEWLUK (see Q2). |
| PATTK.ASM:PATTK | game.cpp cmd_attack | gap-fixed | 6 | Exertion (9-bit sum /8), SNDOBJ, ring auto-hit, darkness 25% gate, KLK2, !!! match. Spent ring was missing PREV00/OCBFIL, fixed on refinement/ring-charges (Q3). Kill path lacks the PUPDAT SYNC (Q2 class). |
| PATTK.ASM:ATTACK | combat attack_hits | matches | 6 | Fixture-backed (combat fixtures). |
| PATTK.ASM:DAMAGE | combat apply_damage | matches | 6 | SCAL16 magic then physical; fixture-backed. |
| PATTK.ASM:ENDGAM | game.cpp endgame_image, endgame_resume | gap-fixed | 11, 21, 23 | WIZIN (CLR HBEATF) and the closing INIVU added in run 11; messages, torch-only bag, weight 200, level 3, FNDCEL. |
| HUPDAT.ASM:HUPDAX |  | gap-fixed | 1 | Fade pacing set from ROM capture C-17 (9e8d66c, ab91dc7). |
| HUPDAT.ASM:HUPD30 |  | gap-fixed | 1 | 5 jiffies per step [ROM], C-17. |
| HUPDAT.ASM:HUPD40 |  | gap-fixed | 3 | Wake-up fade-in climbs to the saved OLIGHT [SRC], 5 jiffies per step [ROM] C-18; the lighting drift of +1 is washed out by PUPSUB [SRC]. |
| HUPDAT.ASM:HUPD00-20, HUPD90, DEATH | game.cpp update_heart_rate | gap-fixed | 19 | 24-bit division, signed faint (<=3) and wake (>4) tests and BLO death match. DEATH's CLR FAINT was missing (fixed); the AUTFLG restart is D-18, Phase 5b. |
| SOUNDS.ASM:SNOISE | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:SNOUT | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:SNWAIT | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:CSETUP | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:SNENV | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:SNENVA | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:SETNVA | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:SETNVD | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:WHOOSH | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:CHUCK | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:CLANK | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:KLANK | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:KKLANK | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:KLINK | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:CLANG | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:BANG | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:THUD | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:KABOOM | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:BOOMER | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:RATTLE | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:GROWL | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:GRAWL | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:SNARL | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:PSSST | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:PSSHT | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:SQUEAK | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:BEOOP | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:WHOOP | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:PHASER | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:GLUGLG | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:BDLBDL | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| SOUNDS.ASM:MSQUEK | presentation snoise.cpp | matches | 13 | SNDTAB mapping and parameters checked; samples fixture-backed (sounds.json); foreground time is D-4b. |
| PINCAN.ASM:PINCAN | game.cpp cmd_incant, incant_hand | matches | 8 | ADJTAB + FULFLG, K.RING, P.OCXXX+1 match, OCBFIL, A$RING, CLR +1, T.RN15 goes to WINNER. The ROM tries the right hand even after the left incants (BSR, then fall-through), while the core stops after the left. That only differs if both hands hold a ring answering the same word [INF]; not changed. |
| PINCAN.ASM:WINNER | game.cpp incant_hand WINNER | matches | 8 | WIZ2 fade-in, two OUTSTI, BRA *; test_winner. |
| PTURN.ASM:PTURN | game.cpp cmd_turn; sdl_app wipe | gap-fixed | 13 | PREVU turns and SYNC match (D-4a [ROM]). TURN AROUND runs RLTURN twice; the core now reports 16 loops and the desktop draws two wipes. |
| PTURN.ASM:PMOVE | game.cpp cmd_move; sdl_app half-step/wipe | deviation (gap fixed run 9) | 8 | Core: parse, PSTEP, PMOV90 exertion ((POBJWT asr 3)+3) on every path, SYNC; the half-step PUPDAT timing is D-4a [ROM]. **Presentation gap:** a forward or back MOVE draws the half-step view (HLFSTP/BAKSTP PUPDAT) before PSTEP, even when PSTEP then THUDs into a wall. The desktop used to show a half-step only when the position changed; since run 9 the MoveAnimation block carries the direction and a wall bump draws the lunge. A blocked sidestep correctly shows no wipe (BNE PMOV90). |
| PTURN.ASM:PSTEP | game.cpp step_player | matches | 8 | STEPOK; on failure ISOUND A$THUD and the position is kept; PUPSUB redraws the backplane. |
| PUPDAT.ASM:PUPDAX | game.cpp PUPDAX redraw | matches | 2 | HUPD32 clears KBDHDR/KBDTAL; the core leaves the buffer but PLAYER discards every char while fainted (HUMAN.ASM:21), and CLK50 stops polling, so the effect is the same [INF]. |
| PUPDAT.ASM:PUPSUB | game.cpp refresh_light | matches | 2 | PRLITE is non-zero only after ENDGAM (PATTK:158), when there is no torch; the core writes 7/$13 directly and refresh_light cannot run then. |
| PGET.ASM:PGET | game.cpp | gap-fixed | 10 | GET: OFIND at PROW, generic or specific, owner++, OBJWGT, HUPDAT, COMUPD. |
| PGET.ASM:PDROP | game.cpp | gap-fixed | 10 | Owner cleared; row, col and level set; negative weight; COMUPD. |
| PGET.ASM:PSTOW | game.cpp | gap-fixed | 10 | Bag push; COMUPD. |
| PGET.ASM:PPULL | game.cpp | gap-fixed | 10 | Bag unlink, PTORCH cleared if pulled; COMUPD. |
| PGET.ASM:COMUPD | game.cpp | gap-fixed | 10 | STATUS + PUPDAT, now charged (D-15). |
| PUSE.ASM:PUSE | game.cpp cmd_use | gap-fixed | 10 | Torch (two PUPDATs), flasks (no OCBFIL, type goes to T.FLA4, reveal 0, no redraw) and scrolls match; the missing PUPDAT charges are added. Quirk: USC100/200 store MAPFLG before the reveal test, so an unrevealed scroll still sets MAPFLG; no visible effect, because every map display runs through a scroll that resets it [INF]. |
| PCLIMB.ASM:PCLIMB | game.cpp cmd_climb | gap-fixed | 11 | VFIND/DIRTAB/VF codes match. The INIVU after NEWLVL was missing, so a climb left the text area uncleared, skipped the heart resets and charged no redraw; added. |
| PREVEA.ASM:PREVEA | game.cpp cmd_reveal | matches | 12 | REV*25 against PPOW (signed BGT; the core's unsigned compare agrees while PPOW < $8000, which the PATT42 cap keeps [INF]), OCBFIL, CLR P.OCREV, STATUS only (no PUPDAT). |
| PEXAM.ASM:PEXAM | game.cpp cmd_examine | gap-fixed | 12 | The PUPDAT after DSPMOD = EXAMIN was uncharged; now D-15. |
| PEXAM.ASM:EXAMIN | presentation examine.cpp project_examine | matches | 12 | Header at 10, creature line at +11, OFIND floor list in two columns (NEWLIN), 32 × '!', backpack header at +12, bag with the lit torch inverted. A trailing newline after an odd bag list was removed in run 12, since EXAM30 has no CRLF. |
| PEXAM.ASM:PRTOBJ | examine.cpp print_names | matches | 12 | COM NEWLIN, tab to the next 16 columns or CRLF; inverse reset after each name. |
| HUMAN.ASM:PLAYER | game.cpp task_player | matches | 12 | Chars eaten while fainted; SP, CR and BS mapped; A-Z to 5 bits; anything else, lowercase included, becomes a space; ROM-captured t4/t5. AUTFLG demo path not modelled (Original Mode starts past the demo). |
| HUMAN.ASM:HUMAN | game.cpp feed_char | matches | 12 | HMAN10 INIVU on any key in map mode; dispatch when the buffer is full; BS at the start ignored; HMAN70 prompt rules. |
| MISC.ASM:WIZIX | src/platform/sdl_app.cpp (death block) | gap-fixed | 1 | 18 jiffies per step [ROM], C-17; still open: NOISEF buzz. 17 WIZZES steps, each ZFLOP+VCTLST+SYNC; 110 ms/step is [INF]; NOISEF 30 Hz buzz, with NOISEV set to the fade value, not reproduced. |
| COMPLR.ASM:HSLOW | game.cpp task_hslow | matches | 3 | ASRD6 of -PDAM, signed BGT floor at 0, reschedule HEARTR on Q.JIF; the core's delay-0 guard needs D > P, which is already death [INF]. C-18 shows healing continuing while fainted. |
| STATUS.ASM:STATUX, OBJNAM | presentation text.cpp project_text, object_name | matches | 14 | Left name at 0, right name flush to column 31 (STAT10: 33 minus length plus terminator), EMPTY name, adjective shown only when revealed. The core clips at 15 characters; no name exceeds 14 [INF]. |
| MAPPER.ASM:MAPPER | presentation mapper.cpp project_map, rasterize_map; sdl_app | gap-fixed | 15 | The logic matched in project_map, but the window never drew the map; rasterize_map added. Text bands hidden in map mode [INF]. |
| PEXAM.ASM:EXAMIN (window) | examine.cpp paint_examine; sdl_app | gap-fixed | 16 | TXTEXA page over the viewport, lit torch inverse; test_examine_page. |
| PZTAPE.ASM:PZSAVE, PZLOAD, FILNAM; COMMON.ASM:SAVE, LOAD, LOAD90 | game.cpp cmd_zsave, cmd_zload, tape_operation | gap-fixed | 17 | FILNAM/GETTOK 8-character name matches; the ??? for a missing name is D-11. LOAD90's PROMPT was missing (fixed in a1df32c). |
| NEWLVL.ASM:NEWLVX, NLVL30-44, NLVL50 | game.cpp build_level, population.cpp birth_creatures/attach_objects; raster.cpp apply_vdginv | matches | 18 | SYSTCB, DGNGEN, births from CTYPES-1 down, round-robin attach (no-live-creature hang is a quirk). NLVL50 polarity done runs 17-18. |
| COMPLR.ASM:BURNER | game.cpp task_burner | gap-fixed | 20 | Timer, dead-torch at <=5, light clamps match. BURN99 DEC NEWLUK was missing (fixed; D-15 baselines regenerated, [ROM]-consistent with t1). |
| COMPLR.ASM:LUKNEW | game.cpp task_luknew | matches | 20 | NEWLUK or map mode, CLR NEWLUK, PUPDAT, SCHED$ 3,Q.TEN. |
| MISC.ASM:WIZIX, WIZOX, WIZZES, WAITX, PREPAX | game.cpp wizard_fade_in, endgame_image/endgame_resume | gap-fixed | 21 | ENDGAM's 1+81+16 SYNCs, both A$EXP1 and WIZIX0 CLRPRI were missing; NEWLVL 3 now uses the later SECOND. DEATH's 17-step fade stays D-14. Note: no test yet for DEATH's CLRPRI. PREPAX not yet traced to a caller. |
| PCLIMB.ASM:PCLI20 + MISC.ASM:PREPAX | game.cpp cmd_climb, begin_newlvl; sdl_app PREPARE! | gap-fixed | 22, 26 | PREPAR's PREPARE! and a timed NEWLVL (D-19) since run 26 (Q6, C-22); descend-early's levels 1 and 0 match the ROM. |
| OBIRTH.ASM:OBIRTX, GENVAL | population.cpp make_object | matches | 24 | Specific OCBFIL, then for GENVAL classes (shield, sword, torch) a generic OCBFIL that keeps P.OCREV and P.OCTYP; special bytes kept when the generic type has no XXXTAB entry. |
| OBIRTH.ASM:OCBFIX | population.cpp ocbfil, fill_ocb_specific | matches | 24 | ODBTAB copy of OD.LEN bytes; OFIL10 writes P.OCXXX only on an XXXTAB match (run 6 fix). |
| TOKEN.ASM (all tables) | lexicon_tables.hpp (generated from tokens.json) | matches | 24 | Fixture-generated from the listing (EXPAND.ASM decode); `make fixtures` checks it. |
| COMTXT.ASM:TXTXXX, TXTBS, TXTCR, TXTSCR; TXTSER.ASM:TXTCHR, TXTSTR, TXTSTI | game.cpp out_char, text; examine.cpp Pad | gap-fixed | 25 | Primary text matches (BS wrap to 127, CR, scroll after the char). The EXAMINE pad dropped lines past row 19 instead of scrolling (fixed); the tab-to-608 status-line spill is a recorded quirk, not reproduced. TXTDPB glyphs are fixture-backed (phase 6). |
| ONCE.ASM:GAME10-GAME50, SYSTCB, IRQSYN | game.cpp Game(), systcb, build_level | gap-fixed | 27 | Start position, PPOW, level-0 build, GAMDAT bag (specific refill is a no-op for wooden/pine), reveal, PROMPT match. P.OCLVL $0B quirk now reproduced. Demo/autoplay path (GAME40) not modelled. |
| CLEAR.ASM:CLRSTX, CLRPRX, CLEAR, ZFLIPX | game.cpp clear_primary_text; raster/apply_vdginv | matches | 28 | CLRPRI homes the cursor and fills with P.TXINV; the core clears to 0 and the window applies polarity (run 17-18). CLRSTS: status line not modelled as a buffer. |
| RANDOM.ASM:RANDOX | rng.hpp Rng::next | matches | 28 | Eight rounds of the $E1 feedback parity into a 24-bit ROL chain SEED, SEED+1, SEED+2; returns SEED. ROM-confirmed by DGEN90 seed captures (phase 1). |
| PLOOK.ASM:INIVUX, PLOOK | game.cpp inivu | matches | 11, 28 | Reviewed in run 11 (CLRPRI, HUPDAT, heart flags, viewer, PUPDAT). |
| PARSER.ASM:GETTOK, PAROBJ, PARSER | parser.cpp | matches | 17, 28 | GETTOK reviewed in run 17; the parse tables and prefixes are fixture-backed (phase-0b tokens.json, parser-prefixes.json). |
| VARC, VERT, VOBJ, D3, D4, DTABAS (FWDOBJ/FWDCRE), SWCHAR (FLATAB), VIEWER (NORSCL/HLFSCL/BAKSCL), VCTLST (SETFAX), VECTOR (BITMSK) data | presentation vctlst/raster from vectors.json | matches | 29 | Fixture-extracted from the listing (phase-6 vectors.json, `make fixtures`); draw-level fixtures per level and light. |
| DGNGEN.ASM | maze.cpp generate_level | matches | 29 | Maze bytes match ROM dumps for levels 0-4 (phase 1, C-22 maze.bin); spin draws and seeds ROM-confirmed. |
| remaining .ASM files | | unreviewed | | Left: VIEWER/VECTOR/VCTLST code paths (drawing logic beyond the fixtures), CD/COMSWI/DAGGORATH/KSK (definitions only). |
 | unreviewed | | Split into labels when reached. |

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
| 2026-09-27 | 9 | PMOVE wall bump | The desktop now draws the half-step before the THUD; the MoveAnimation block carries step_relative; test added | this commit | — |
| 2026-09-27 | 10 | PGET (GET, DROP, STOW, PULL, COMUPD), PUSE | Logic matches; COMUPD, PUSE and PATT40 PUPDATs were uncharged | D-15 extended; fight baselines regenerated (one kill SYNC each); 3 tests; audited | Tests for GET, DROP and STOW individually — not added (the shared COMUPD path is covered by PULL) |
| 2026-09-27 | 11 | PCLIMB, INIVU/PLOOK, HMAN10, endgames | Typing after a map never left map mode; CLIMB skipped INIVU; INIVU lacked CLRPRI, PLOOK and PUPDAT; the ring riddle wrongly stopped the heartbeat (a stray WIZIX CLR HBEATF); ENDGAM lacked WIZIX's CLR HBEATF | Fixed; tests for climb, map exit, riddle and ENDGAM (no LOAD90 test); no baseline moved; Q4 raised | GAME50 charge — not run: Q4 |
| 2026-09-27 | 12 | PEXAM, EXAMIN, PRTOBJ, PREVEA, HUMAN | The PEXAM PUPDAT was uncharged; the rest match | PEXAM charge + test; no baseline moved | — |
| 2026-09-27 | 13 | SOUNDS.ASM (all 31 routines), PTURN, STEP, ENDGAM | SOUNDS matches in full; TURN AROUND's second RLTURN sweep was missing on the desktop | About-face wipe drawn twice (16 loops) + test | — |
| 2026-09-27 | 14 | Audit of run 13; STATUS.ASM | Run 13 audit clean; D-4a wording fixed (8 TURN10 iterations, 16 for an about-face, [INF] outside the viewer); STATUS matches | docs + a sturdier sweep count | — |
| 2026-09-27 | 15 | MAPPER | The window showed a blank viewport in map mode | Map raster + test; examine screen still not drawn (gap-open) | Examine window drawing — next run |
| 2026-09-27 | 16 | EXAMIN in the window | The window showed a blank viewport in examine mode | paint_examine + shared examine_snapshot_from + test | — |
| 2026-09-27 | 17 | PZTAPE, COMMON SAVE/LOAD/LOAD90 | LOAD90 PROMPT missing after INIVU; the audit of a1df32c found its reverse-video band disagrees with NLVL50 (Q5) | Prompt fix + test (committed by Steve in a1df32c) | a1df32c audited; band left to Steve |
| 2026-09-27 | 18 | NEWLVL; Q5 follow-up | NEWLVL matches; the VDGINV flip moved into a tested presentation helper | apply_vdginv + test | none |
| 2026-09-27 | 19 | HUPDAT heart rate, HUPD90, DEATH | DEATH did not clear FAINT; the halt-instead-of-restart was unrecorded | CLR FAINT + test; D-18 added | none |
| 2026-09-27 | 20 | COMPLR BURNER, LUKNEW | BURNER never set NEWLUK | NEWLUK at BURN99 + test; t1-t5 and phase-3 fight traces regenerated under Q2 | f53cf22 was pushed with a failing test: the gate chain used `grep`, which succeeded on the FAIL line. Fixed in the next commit. Gate on `make all`'s exit code, never on grep. |
| 2026-09-27 | 21 | MISC wizard/WAIT routines, ENDGAM timing; audit of 66e2b6f | ENDGAM ran instantly: no WIZIN/WAIT/WIZOUT SYNCs, no A$EXP1, level 3 built ~1.4 s early (different SECOND) | Staged ENDGAM + timing test; no baseline moved | — |
| 2026-09-27 | 22 | PREPAR/PCLIMB build time; ENDGAM text | [ROM] CLIMB builds ~330 interrupts after the command, so level SECOND and creature placement differ (Q6, C-22). A test for WIZIN's CLRPRI showed DEATH's is unobservable (the faint's CLRPRI always runs first), and ENDGAM's staged text shows an unexplained extra dot row before its messages: likely the staged ENDGAM lets HMAN70's line handling run early (gap-open, next run). | none; edits reverted | ENDGAM text — not run: time |
| 2026-09-27 | 23 | ENDGAM extra prompt line (from run 22) | Staged ENDGAM let HMAN70 prompt before its stages; INIVU then wiped it, leaving no prompt | HMAN70 deferred to ENDGAM's last stage + tests; audited | — |
| 2026-09-27 | 24 | OBIRTH, TOKEN | Both match (TOKEN via fixtures) | log only | — |
| 2026-09-27 | 25 | COMTXT, TXTSER | EXAMINE page never scrolled | Pad scroll + test; quirk recorded; audited | — |
| 2026-09-27 | 26 | Q6: CLIMB/ENDGAM NEWLVL build time | C-22 captured (levels 1-4); timed NEWLVL + PREPARE! applied; SYSTCB tasks deferred to the build's end after the capture showed CREGEN must follow the births | Core, window, tests, reconciliation | — |
| 2026-09-27 | 27 | ONCE game start | Starting objects' level byte was 0, listing leaves $0B | Quirk reproduced + test; audited | — |
| 2026-09-27 | 28 | CLEAR, RANDOM, PLOOK, PARSER | All match | log only | — |
| 2026-09-27 | 29 | Vector data files, DGNGEN | Covered by listing-extracted fixtures and ROM maze dumps | log only; second run in a row with no finding — stop suggested | — |
