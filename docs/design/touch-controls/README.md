# Touch controls: agreed design direction

Design input for Phase 8 ([`../../prompts/phase-8-touch-input.md`](../../prompts/phase-8-touch-input.md)).
Mockups: `mockups/*.dc.html` (Design canvas source; `canvas.json` is the board layout).
These are hypotheses to implement and evaluate, not evidence about the original.

## Layout

- **Landscape, thumbs on the sides.** The game renders full screen at 4:3, full
  height, centred. Controls are an overlay on top of it. On 19.5:9 phones they
  sit in the black side margins; on 16:9 they overlap the corridor edges but
  stay clear of the centre.
- **Style follows the original:** square buttons, white outline on black, one
  capital letter or an arrow; a pressed button inverts. No hand colours or
  labels: the game's own status line already shows what each hand holds.

| Position | Button | Shown when | Keystrokes |
|---|---|---|---|
| Top corners | `A` (left / right) | always (empty hand allowed) | `A L` / `A R` |
| Under each `A` | `G` | that hand empty | `G L` → floor picker → `G L <obj>` |
| Under each `A` | `P` | that hand empty | `P L` → pack picker → `P L <obj>` |
| Under each `A` | `≡` menu → `S D U R I` | that hand holding | `S L` · `D L` · `U L` · `R L` · `I …` |
| Bottom left | `↑ ↓ ⇤ ⇥` | always | `M` · `M B` · `M L` · `M R` |
| Bottom left | `↶ ↷ ↻` | always | `T L` · `T R` · `T A` |
| Bottom right | `C` → choose `U` / `D` | on ladder or hole | `C U` / `C D` (both always offered, as confirmation) |
| Bottom right | `E`, `L` | always | `E` · `L` |
| Bottom right | `⌨` | always | free command line (ZSAVE, ZLOAD, anything) |

## Interaction rules

- **Sequential entry.** Every tap types its letters into the real command line,
  so a partial command appears exactly as typed input would (`.G L` while the
  picker is open). The typed command line stays available and authoritative.
- **No pausing in play.** Pickers and the keyboard never stop the clock. Only
  the shell's system menu pauses (ADR-0009, D-16).
- **INCANT.** `I` types `I ` and opens an in-game A–Z keyboard (QWERTY, `⌫`,
  `↵`) with a clear, empty text box and no hint. The player types the word and
  presses `↵`. The in-game keyboard replaces the system keyboard, which would
  be more disruptive. `✕` cancels. The same keyboard backs `⌨`.
- **RESTART** is not a command in `CMDTAB`; not provided.

## Decided 2026-09-27

- **System menu button** at bottom right, with `C E L ⌨` (ADR-0009 §6). It is
  the only control that pauses.
- **Whole line at once.** A finished gesture delivers its command line to the
  keyboard buffer on one jiffy (D-17). Partial lines still show while a
  picker or keyboard is open.
- **Tablet 4:3.** Controls float over the game's left and right edges, not
  the bottom, which stays clear for the status and command lines. The hand
  buttons use the mostly empty upper left and right of the viewer.
- **Pickers show only what is visible**: floor objects drawn on the player's
  cell now (none in the dark), and pack names as the EXAMINE page gives them.
  To be checked against the listing in 8.0.

