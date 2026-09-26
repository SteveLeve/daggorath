# Progression and save

**[SRC]** unless noted.

## Endings

Killing creature type 10 runs `ENDGAM` (`PATTK.ASM`). The lit torch, if any, becomes the only bag object. Hands are cleared. Carried weight is set to 200. Level 3 is rebuilt with `NEWLVL`. `FNDCEL` picks a non-`$FF` cell for the player.

Killing creature type 11 freezes creatures (`DEC FRZFLG`), sets `PRLITE` to `$0713`, and clears the bag, torch, and both hands. The omega ring is not created here; `OCBPTR` is rewound in the listing, which this core does not model as a flat object arena.

`INCANT` of the full word stored in a held ring's `P.OCXXX+1`, when that word is the final ring, runs `WINNER` and halts (`BRA *`).

Death remains the Phase 3 `PPOW < PDAM` halt. The death line is recorded as dialogue state. Drawing it is later.

## Save

See ADR-0005. `ZSAVE` stores the modeled `$0200`–`MM.END` fields under the filename token (eight characters, `PZTAPE.ASM`). `ZLOAD` restores the newest matching name. A missing name is `???`.
