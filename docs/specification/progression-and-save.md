# Progression and save

**[SRC]** unless noted.

## Endings

Killing creature type 10 runs `ENDGAM` (`PATTK.ASM`). Hands are cleared and the bag is replaced by the object in `PTORCH` when that index is set, including a dead torch. Whether the listing keeps only a lit torch is unresolved. Carried weight is set to 200. Level 3 is rebuilt with `NEWLVL`. `FNDCEL` rejects a `$FF` cell. The loop draws a column, then a row, and draws both again after a reject. That order is unresolved against the listing.

Killing creature type 11 freezes creatures (`DEC FRZFLG`), sets `PRLITE` to `$0713`, and clears the bag, torch, and both hands. The omega ring is not created here; `OCBPTR` is rewound in the listing, which this core does not model as a flat object arena.

`INCANT` of the full word stored in a held ring's `P.OCXXX+1`, when that word is the final ring, runs `WINNER` and halts (`BRA *`).

Death remains the Phase 3 `PPOW < PDAM` halt. The core emits the dialogue string `YET ANOTHER DOES NOT RETURN`. That string is not cited from the pinned listing in this repository, so the wording is unresolved. Drawing it is later.

## Save

See ADR-0005. `ZSAVE` stores the modeled `$0200`–`MM.END` fields under the filename token (eight characters, `PZTAPE.ASM`). `ZLOAD` restores the newest matching name. A missing name is `???`.
