# Commands and parser

Living specification for the command language the Phase 0b core implements, and
for the token rules the later commands will share. Labels are source-proven
unless marked otherwise. Token tables and the matching rule are source-proven.
Dispatch timing for `MOVE` and `TURN` is ROM-observed in
[`../archaeology/phase-1/reconciliation.md`](../archaeology/phase-1/reconciliation.md) §1
and is not restated here.

## 1. Token tables

**[SRC]** `TOKEN.ASM`, decoded by the `EXPAND.ASM` 5-bit packing. Four tables
are searched: `CMDTAB`, `DIRTAB`, `ADJTAB`, `GENTAB`. `DEBFLG` is 0, so the
debug commands (`QMAP` and the rest) are absent.

`CMDTAB` has 15 verbs: `ATTACK`, `CLIMB`, `DROP`, `EXAMINE`, `GET`, `INCANT`,
`LOOK`, `MOVE`, `PULL`, `REVEAL`, `STOW`, `TURN`, `USE`, `ZLOAD`, `ZSAVE`.
There is no `RESTART`, `SETOPT`, `SETCHEAT`, or a bare `BACK` verb.

The shortest unique prefix of every verb is one letter, except `ZLOAD` and
`ZSAVE`, which need `ZL` and `ZS`. A bare `Z` matches both and is rejected.

**[SRC]** `DIRTAB` stores `BACK`, four letters. `DIRXXX` in `DTABAS.ASM` names
the macro argument `BACKWARD`; that name is not the packed string. `BACKWARD`
is longer than the entry and is not a prefix of it, so the parser rejects it.

**[SRC]** Ring adjectives come from `ADJTAB`, not from the `OBJXXX` label.
The typed words are `VULCAN`, `RIME`, `JOULE`, `SUPREME`. `HOTH` is the macro
label for the object whose adjective is `RIME`. Revealed names are `FIRE`,
`ICE`, `ENERGY`, `FINAL`, plus `GOLD`.

Fixtures: `fixtures/tokens.json`, `fixtures/parser-prefixes.json`.

## 2. Matching rule

**[SRC]** `PARSER.ASM`. A typed token matches a table entry when it is a prefix
of that entry. If two entries match, the search fails, including the case where
one of the matches is the entire word. `PARFLG` is tested before any full-word
preference. `FULFLG` records a full-word match and only `INCANT` reads it.

In the shipped tables no entry is a prefix of another, so the full-word case is
unreachable. The rule still applies if a word is added.

Trailing tokens are left on the line. Each handler reads only the tokens it
needs. Resetting `LINPTR` discards the rest. That is not an error.

## 3. Dispatch

**[SRC]** `HUMAN.ASM HMAN50`, `DTABAS.ASM DISPAT`.

`PARSER` is called on `CMDTAB`. A null token does nothing and prints no error.
A failed search prints `???` (`CMDERR`). Otherwise the token type indexes
`DISPAT`.

The core dispatches `MOVE`, `TURN`, and `LOOK`. Every other verb emits
`UNIMPLEMENTED` and does not approximate the command.

### MOVE

**[SRC]** `PTURN.ASM PMOVE`, `PSTEP`, `PMOV90`. No direction means forward.
`BACK` is a half-turn, `LEFT` and `RIGHT` are sidesteps. `AROUND`, `UP`, and
`DOWN` are `???`. An unrecognised token is `???`. Every path that accepts the
line, including a blocked step, adds `(POBJWT >> 3) + 3` to `PDAM` and calls
`HUPDAT`. A blocked step plays the thud and does not change `PROW`/`PCOL`.
The command ends in `DEC UPDATE / SYNC`.

### TURN

**[SRC]** `PTURN.ASM PTURN`, `PREVU`. `LEFT`, `RIGHT`, and `AROUND` change
`PDIR` and mask it with 3. `BACK`, `UP`, and `DOWN` are `???` and do not turn.
A missing direction is `???`. The command ends in `DEC UPDATE / SYNC`.

### LOOK

**[SRC]** `PLOOK.ASM`. Selects the forward view (`DSPMOD` = viewer). The core
records the mode change and does not render it.

## 4. Line editing that feeds the parser

**[SRC]** `HUMAN.ASM`, `CD.ASM LINBUF`. The line buffer is 32 bytes. Backspace
does not move past the start. When the buffer fills, `HMAN20` falls through
into the carriage-return path, so the 32nd character dispatches the line
without Return.

**[SRC]** `COMMON.ASM KBDPUT`. The keyboard buffer is 32 bytes and has no
overflow check. A burst of exactly 32 leaves head equal to tail, which reads
as empty. `PLAYER` drains the whole buffer on its turn, so a command typed
inside one jiffy is parsed in that turn. The interrupt still accepts at most
one character per jiffy.

While `FAINT` is set, `PLAYER` still drains the buffer and discards every
character.

Space, Return, and Backspace have the internal codes `I.SP`, `I.CR`, and
`I.BS`. `A`..`Z` become `ASCII & $1F`. Any other byte becomes a space. A
lower-case letter is not a letter.

## 5. Commands not specified here

`ATTACK`, `CLIMB`, `DROP`, `EXAMINE`, `GET`, `INCANT`, `PULL`, `REVEAL`,
`STOW`, `USE`, `ZLOAD`, and `ZSAVE` have decoded names and prefixes. Their
effects are unresolved in this document and are not implemented.
