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
   platform maps them to device pixels and draws them with SDL3 geometry, with
   optional thickness and anti-aliasing. No second geometry source.
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

## Open for Phase 8

- Text and status lines in `crisp`: the original glyph bitmap scaled
  nearest-neighbour, or glyphs as strokes. The first keeps the character
  set exact.
- The map (`MAPPER`) is cell-based: crisp squares at device resolution, or
  pixel style.
- Line thickness default relative to screen size; whether `pixel` offers
  smoothing filters.
