# Touch input: command coverage

Living document for Phase 8. This section (8.0) is the interaction inventory:
every command form the parser accepts, its typed keystrokes, and the touch
control that produces them, per the agreed design
([`docs/design/touch-controls/README.md`](../design/touch-controls/README.md)).
Later workstreams add the adapter, shell, render-style and layout-evaluation
sections named in [`docs/planning/phase-8-plan.md`](../planning/phase-8-plan.md).

Sources: [`docs/specification/commands-and-parser.md`](../specification/commands-and-parser.md)
(token tables, §1; dispatch, §3), [`docs/specification/combat-and-items.md`](../specification/combat-and-items.md)
(`PATTK`, `PGET`/`PDROP`/`PSTOW`/`PPULL`, `PUSE`, `PINCAN`), and the decoded
token fixtures `docs/archaeology/phase-0b/fixtures/tokens.json` and
`parser-prefixes.json`. Labels follow those documents: **[SRC]** is the
listing, **[INF]** is this project's inference, unmarked rows restate a
**[SRC]** row from those documents without new claims.

## 1. Verb inventory (`CMDTAB`, 15 entries) — [SRC]

Every verb the parser accepts, its shortest unique prefix (`parser-prefixes.json`),
what the core currently dispatches (`commands-and-parser.md` §3), and the touch
control that types it.

| Verb | Prefix | Core dispatch | Touch control | Keystrokes produced |
|---|---|---|---|---|
| `MOVE` | `M` | dispatched (`PMOVE`/`PSTEP`/`PMOV90`) | bottom-left `↑ ↓ ⇤ ⇥` | `M` · `M B` · `M L` · `M R` |
| `TURN` | `T` | dispatched (`PTURN`/`PREVU`) | bottom-left `↶ ↷ ↻` | `T L` · `T R` · `T A` |
| `LOOK` | `L` | dispatched (`PLOOK`, selects viewer mode) | bottom-right `L` | `L` |
| `ATTACK` | `A` | `UNIMPLEMENTED` (Phase 3 spec exists, not yet wired to `CMDTAB`) | top-corner `A` (left/right) | `A L` · `A R` |
| `GET` | `G` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `G`, empty hand only → floor picker | `G L <obj>` · `G R <obj>` |
| `PULL` | `P` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `P`, empty hand only → pack picker | `P L <obj>` · `P R <obj>` |
| `STOW` | `S` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only | `S L` · `S R` |
| `DROP` | `D` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only | `D L` · `D R` |
| `USE` | `U` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only | `U L` · `U R` |
| `REVEAL` | `R` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only | `R L` · `R R` |
| `INCANT` | `I` | `UNIMPLEMENTED` (Phase 4 spec exists) | under-`A` `≡` menu, holding hand only → in-game keyboard | `I ` then the typed adjective, e.g. `I VULCAN` |
| `EXAMINE` | `E` | `UNIMPLEMENTED` (Phase 4 spec exists) | bottom-right `E` | `E` |
| `CLIMB` | `C` | `UNIMPLEMENTED` (unresolved effect, §5) | bottom-right `C` → choose `U`/`D`, offered on ladder or hole | `C U` · `C D` |
| `ZLOAD` | `ZL` | `UNIMPLEMENTED` (Phase 4/5 spec exists) | bottom-right `⌨` only (free command line) | `ZLOAD` (or any unambiguous prefix `ZL...`) |
| `ZSAVE` | `ZS` | `UNIMPLEMENTED` (Phase 4/5 spec exists) | bottom-right `⌨` only (free command line) | `ZSAVE` (or `ZS...`) |

Every verb has a touch path. `ZLOAD`/`ZSAVE` are deliberately keyboard-only
(design doc §"Interaction rules"): they need a filename-free confirmation
flow the mockups do not model, and the typed line is always available as the
fallback for every command, including these two.

A bare `Z` matches both `ZLOAD` and `ZSAVE` and is rejected by the parser
(ambiguous prefix, §1); no control types a bare `Z`.

## 2. Direction and adjective tokens (`DIRTAB`, `ADJTAB`) — [SRC]

`DIRTAB`: `LEFT`, `RIGHT`, `BACK`, `AROUND`, `UP`, `DOWN` (prefixes `L`, `R`,
`B`, `A`, `U`, `D`; all shortest-unique at one letter). Which directions a verb
accepts is per-verb, not per-table:

| Verb | Accepted directions | Rejected (`???`) | Touch source |
|---|---|---|---|
| `MOVE` | none (forward), `BACK`, `LEFT`, `RIGHT` | `AROUND`, `UP`, `DOWN`, unrecognised | `↑`/`⇤`/`⇥` type `M`/`M L`/`M R`; `↓` types `M B` |
| `TURN` | `LEFT`, `RIGHT`, `AROUND` | `BACK`, `UP`, `DOWN`, missing | `↶`/`↷`/`↻` type `T L`/`T R`/`T A` |
| `ATTACK` | `LEFT`, `RIGHT` (hand, via `PARHND`) | anything else | `A` buttons type `A L`/`A R` |
| `CLIMB` | unresolved (§5); design assumes `UP`/`DOWN` from the ladder/hole context | — **[INF]** not yet checked against a `CLIMB` listing routine | `C` button offers `C U`/`C D` |

`ADJTAB` (25 entries) covers ring adjectives (`VULCAN`, `RIME`, `JOULE`,
`SUPREME`) and item adjectives (`ELVISH`, `MITHRIL`, `SEER`, `THEWS`, `VISION`,
`ABYE`, `HALE`, `SOLAR`, `BRONZE`, `IRON`, `LUNAR`, `PINE`, `LEATHER`,
`WOODEN`) plus the four revealed ring names and `GOLD`, `EMPTY`, `DEAD`. The
`INCANT` in-game keyboard (design doc) accepts any of these; the parser
rejects anything that is not a full adjective (`PINCAN` "requires the full
adjective", `combat-and-items.md`).

`GENTAB` (6 entries): `FLASK`, `RING`, `SCROLL`, `SHIELD`, `SWORD`, `TORCH` —
the generic names `GET`/`PULL` pickers must offer, per object class.

## 3. Line editing and the 32-byte buffer — [SRC] / [INF]

**[SRC]** `commands-and-parser.md` §4: the line buffer is 32 bytes; the 32nd
character dispatches without Return; the keyboard buffer is also 32 bytes with
no overflow check, and a burst of exactly 32 leaves head equal to tail (reads
as empty — the buffer-overrun quirk).

**[INF]** deviation D-17 (planned) requires every touch line to be at most 31
characters including the terminating CR, so a touch gesture cannot by itself
produce the 32-character overrun; that stays reachable only by typing. The
longest touch-composed line here is `INCANT` with a 7-letter adjective:
`I SUPREME\r` = 10 bytes, well under the limit. The overrun is preserved as a
typing-only path (`⌨`), not removed as a possibility — 8.1 must show it is
still reachable by typing through the same keyboard buffer the touch adapters
feed.

## 4. Picker contents — [INF], to close in implementation

Per the design doc's decision: floor and pack pickers show only what the
player can see. **[INF], not yet checked against the listing (open item
carried from the plan):**

- Floor picker: objects drawn on the player's current cell, none while dark.
  The exact "objects on this cell" read needs a core accessor (`combat-and-items.md`
  `PGET`/`PDROP` describe the floor-object list but not a query API); if
  missing, add it as an isolated `src/core` read-only-accessor commit per the
  goal's rule, with its own boundary check.
- Pack picker: names as the `EXAMINE` display gives them (`combat-and-items.md`
  `USE`/`EXAMINE`); `EXAMINE`'s display format is otherwise unresolved in the
  specification (`commands-and-parser.md` §5 lists it as not yet specified),
  so the exact name strings are an 8.1/8.4 implementation detail, not decided
  here.

## 5. Commands with unresolved effect

`ATTACK`, `CLIMB`, `DROP`, `EXAMINE`, `GET`, `INCANT`, `PULL`, `REVEAL`,
`STOW`, `USE`, `ZLOAD`, `ZSAVE` are `UNIMPLEMENTED` in the core today
(`commands-and-parser.md` §3, §5). The coverage table above is a contract for
touch to reach the same keystrokes a typist would use; it does not require the
core to implement these verbs, and does not approximate them. Phase 8 adds no
gameplay behaviour (`docs/prompts/phase-8-touch-input.md`: "do not build...
rule changes").

## 6. Coverage gate

Every one of the 15 `CMDTAB` verbs has at least one touch path to its
keystrokes, or is explicitly keyboard-only with a stated reason (`ZLOAD`,
`ZSAVE`, §1). This closes the 8.0 completion gate ("coverage table: every
command form reachable by touch") for the command language as currently
specified. Two follow-ups are carried into later workstreams, not reopened
here:

- §4's floor/pack visibility rules, checked against the listing when `GET`/
  `PULL`/`EXAMINE` are implemented (outside Phase 8's scope per the prompt).
- §2's `CLIMB` direction handling, checked against the listing when `CLIMB` is
  implemented.

## 7. Crisp render style (8.3, ADR-0010)

`src/presentation/crisp.{hpp,cpp}`, headless (no SDL). `build_crisp_frame`
consumes `RenderState::segments` — the same logical draw list `pixel`'s
`raster.cpp` rasterises (ADR-0010 §2), not a second geometry source. A
fully-lit segment (`fade == 0`) becomes one continuous line; a dim segment
becomes the exact dots `draw_segment` would plot (found via the newly shared
`raster.hpp::walk_segment`), matching ADR-0010 §8's dotted dimness. `build_crisp_map`
projects the same `MapSnapshot` `rasterize_map` (`mapper.hpp`) rasterises: a
solid-wall cell becomes one filled square (§7), and the player/object/
creature/vertical-feature marks stay nearest-neighbour bitmap cells (small
bitmaps, not squares) via the newly shared `mark4_rows`.

Segment fixtures, one per Phase 6 golden state:
`docs/archaeology/phase-8/fixtures/crisp-segments.txt`, regenerated and
checked byte-for-byte by the `crisp_segment_fixtures` ctest. The Phase 6/7
golden pixel images are unchanged — confirmed by `viewer_regressions`,
`text_regressions`, and `make verify`'s fixture manifests, all still passing
after the `walk_segment`/`mark4_rows` refactor that let both render styles
share one implementation.

Not built here: device-pixel scaling, line thickness, HiDPI handling, and
smoothing are SDL3 platform concerns this headless module leaves to the
caller (it emits source-256x192 coordinates only) — deferred to 8.4, which
wires `crisp`/`pixel` into `src/platform`. Text stays on the existing bitmap
path (`text.cpp`); ADR-0010 §6's all-vector-text option is not reopened.

## Later sections (added by 8.4–8.5)

- Layouts evaluated and the chosen default, with screenshots kept out of tree
  (8.4).
- Replay-equivalence evidence (8.5).
