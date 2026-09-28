# ADR-0010 — Render styles: crisp vectors and the exact raster

**Status:** Proposed, 2026-09-27. Settled by Phase 8 workstream 8.3.

**Resolution (2026-09-27, Phase 8.3):** Accepted as written, for the part
built headlessly in `src/presentation`:

- `crisp.hpp`/`crisp.cpp` (`build_crisp_frame`) consumes `RenderState::segments`
  — the same logical draw list `pixel`'s `raster.cpp` rasterises (§2). A
  fully-lit segment (`fade == 0`) becomes one `CrispLine`; a dim segment
  (`0 < fade < 0xFF`) becomes the exact dots `draw_segment` would plot,
  found via `raster.hpp`'s new `walk_segment` (the same VECTOR.ASM walk,
  refactored out of `draw_segment` so there is one implementation, not a
  second geometry source) — §8's dotted dimness, no colour blend.
- `build_crisp_map` projects the same `MapSnapshot` `rasterize_map`
  (mapper.hpp) rasterises: a solid-wall cell becomes one filled square (§7);
  the player/object/creature/vertical-feature marks become nearest-neighbour
  bitmap cells (`mapper.hpp`'s new shared `mark4_rows`, also now the single
  source `rasterize_map` itself calls) — the same choice §6 makes for text,
  extended here because those marks are small bitmaps, not solid squares.
- **Segment fixtures, one per Phase 6 golden state** (`docs/archaeology/phase-8/fixtures/crisp-segments.txt`,
  regenerated and compared byte-for-byte by `tests/CMakeLists.txt`'s
  `crisp_segment_fixtures`), demonstrating `crisp` and `pixel` project the
  same data.
- **Golden images unchanged:** `viewer_regressions`, `text_regressions`, and
  the three fixture manifests (`make verify`) all still pass after the
  `walk_segment`/`mark4_rows` refactor, with no fixture regenerated.

**Not yet built, and not claimed here:** §6's text-as-lines question is not
reopened (still "a later option, not Phase 8" per that section) — this PR
adds no glyph-cell geometry at all, vector or bitmap; the existing bitmap
text path (`text.cpp`) is untouched. Device-pixel scaling, line thickness,
HiDPI handling, and whether smoothing helps (`raster.hpp`'s "Open for
Phase 8" section) are SDL3 platform concerns this headless module
deliberately leaves to the caller (it emits source-256x192 coordinates
only); they are deferred to 8.4, which wires `crisp`/`pixel` into
`src/platform`. This Resolution settles the geometry source and the segment
contract, not the on-screen rendering.

**Addendum (2026-09-27):** SDL3 became available (see
`docs/archaeology/phase-8/reconciliation.md`'s addendum) and workstream 8.6
wired the touch overlay and shell into `src/platform/sdl_app.cpp`, but the
`crisp`/`pixel` device-rendering toggle this Resolution defers to "8.4" was
explicitly time-boxed out of that session (tracked as 8.6.3) and not
attempted. `pixel` (the exact bitmap raster) remains the only on-screen
render style; this Resolution's open items are all still open.

**Addendum (2026-09-27, workstream 8.6.3):** the toggle above was built in a
follow-up session. `src/platform/sdl_app.cpp`'s F1 key switches `present_frame`'s
overlay callback between doing nothing (`pixel`, default, unchanged texture
blit) and overdrawing the viewport band with `draw_crisp_view`/`draw_crisp_map`
— `SDL_RenderLine`/`SDL_RenderFillRect` calls scaled by the same integer
`kScale` `pixel` uses, fed from `dag::project(snap)`/`build_crisp_frame` and
`build_crisp_map` directly (no second geometry source, per §2). Verified with
a temporary, since-removed light-forcing hook (`ViewSnapshot::regular_light`/
`magic_light` set to the same values `tests/presentation/crisp_fixture_gen.cpp`
already uses for its golden states — the real power-on view is dark until a
torch is lit, which no implemented command can do yet): screenshots of both
styles at the same forced-light state show the same corridor geometry
(`captures/phase-8-sdl-wiring/{crisp,pixel}-corridor-2026-09-27.png`,
gitignored). Golden-image tests (`viewer_regressions`, `text_regressions`,
`crisp_segment_fixtures`, `make verify`) are untouched and still pass, since
`pixel` stays the default and no fixture was regenerated. **Not built:** line
thickness/smoothing (still "Open for Phase 8" above) — lines are drawn at
1 device pixel regardless of screen size.

## Context

The desktop window rasterises the viewer's vector draw list onto a 256×192
surface (`src/presentation/raster.cpp`, the `VECTOR` DDA) and scales it by an
integer factor (`scale_frame`, used in `src/platform/sdl_app.cpp`). That is the
exact CoCo image and the reference for golden-image tests. On a phone it reads
as blocky. The web port's vector mode draws the same geometry as lines at
display resolution and looks sharp. ADR-0007 rule 2 allows presentation options
in every mode.

## Decision

1. Two styles: **`crisp`** (default) and **`pixel`** (the exact raster).
2. `crisp` consumes the **same logical draw list** `pixel` rasterises: line
   segments in 256×192 source coordinates, including `VCTFAD` and scale. The
   platform maps them to device pixels and draws them with SDL3 geometry. No
   second geometry source. Lines are drawn as quads whose thickness scales
   with the device, and all sizing uses real device pixels (SDL3 pixel
   density), not logical points. The
   [vector study](../planning/port-comparison.md#vector-rendering-study-2026-09-27)
   found that the web port's fixed 1-pixel lines and missing HiDPI handling
   are what this avoids.
3. Golden images stay on `pixel`. `crisp` gets a segment-list fixture per
   golden state, so both styles are demonstrably one projection.
4. Every frame the window draws — viewer, map, examine page, turn wipe (D-13),
   half-steps, faint fade and wizard (D-14) — must have a segment or cell form.
   Phase 8 inventories them before implementation.
5. **Provenance.** The idea comes from the web port's vector mode, read as a
   reference ([port comparison](../planning/port-comparison.md),
   [ledger](../provenance/ledger.md)). The geometry is this project's existing
   draw list, derived from the listing's `VECTOR`/`VCTLST` data (ADR-0006). No
   code from the Hunerlach or web ports enters the tree.

6. **Text in `crisp`** (decided 2026-09-27): status line, command line and
   text pages use the original glyph bitmaps, scaled nearest-neighbour; only
   the vector geometry is drawn as lines. The web port reaches the same look by
   drawing each glyph cell as a filled quad in every mode (vector study). Reason: the
   geometry is the listing's `VECTOR`/`VCTLST` data (ADR-0006), so drawing it
   as lines renders the same data more sharply. **[INF]** the text glyphs are
   bitmaps (the Phase 6 text fixtures). Stroked text would need glyph data the
   original never had. That would be a new
   design with its own provenance, so it is left as a later option.

7. **Map in `crisp`** (decided 2026-09-27): `MAPPER` cells are drawn as
   sharp squares at device resolution, from the same cell projection
   (`mapper.hpp`) that `pixel` rasterises.

8. **Dimness in `crisp`** (decided 2026-09-27): `VCTFAD` keeps the
   listing's dot-skipping: **[SRC]** `VECTOR.ASM` plots one dot in every
   `VCTFAD`+1 steps of its walk, as transliterated in `raster.cpp`
   `draw_segment`, whose output the Phase 7 golden images fix. A dim segment is drawn as evenly spaced sharp dots
   at the spacing the `pixel` DDA uses (`raster.cpp`), not as a colour blend.
   The web port's blend is rejected, because it changes how dim creatures
   look.

## Open for Phase 8

- Whether smoothing helps at the chosen thickness. The web port disabled it
  without a stated reason. Evaluate in 8.3.

- All-vector text (see §6): later option, not Phase 8.
- Line thickness default relative to screen size; whether `pixel` offers
  smoothing filters.
