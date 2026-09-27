# ADR-0010 — Render styles: crisp vectors and the exact raster

**Status:** Proposed, 2026-09-27. Settled by Phase 8 workstream 8.3.

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
