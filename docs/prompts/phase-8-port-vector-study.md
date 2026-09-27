# Phase 8 — port vector rendering study (run locally)

Reference study for the preservation project's Phase 8 crisp renderer
(ADR-0010). Read-only. **Copy no code into any file, and quote no more than a
few lines needed to name a technique.** Report techniques and reasons in your
own words; the preservation project reimplements from its own draw list.

**Sources, in order of preference** (record which you actually read):
1. `DungeonsOfDaggorath.github.io` checkout: `git rev-parse HEAD`; check whether
   the `cognitivegears/DungeonsOfDaggorath` submodule is populated
   (`git submodule status`). If it is, that C++ is the web build's source.
2. Otherwise `BlatantlyX/DungeonsOfDaggorath` (SDL2 port the WASM derives from),
   cloned read-only anywhere outside the preservation repo. Record the commit.
3. Do not disassemble the `index.wasm`.

**Answer each, citing file:line and commit:**
1. **Pipeline.** Where the viewer's vector lists are turned into draw calls;
   what API (GL_LINES, quads, SDL_Render…); immediate or batched.
2. **Coordinates.** Source coordinate space (256×192?) and how it maps to the
   window: aspect ratio (the `width*0.75` height rule), letterboxing, integer
   vs fractional scaling, pixel-centre offsets.
3. **Line quality.** Line width and how it scales with resolution; anti-aliasing
   (GL_LINE_SMOOTH, MSAA, blending); line caps/joins; how very short lines and
   single dots are drawn.
4. **Graphics modes.** What NORMAL, HIRES and VECTOR each do, and what differs
   between them in code (same geometry, different rasterisation?).
5. **Text and status line.** How glyphs are drawn in VECTOR mode (bitmap
   texture, quads per pixel, strokes) and any comment explaining why text stays
   pixel style.
6. **Map.** How the map screen is drawn in each mode.
7. **Fades and brightness.** How creature fade-in (`VCTFAD`-like), the faint
   fade and the death wizard are done: skipped lines, alpha, colour, timing.
8. **Scaling of creatures/objects by distance.** Where the scale is applied
   (in geometry, or matrix transform), and rounding.
9. **Web/Emscripten specifics.** Canvas sizing, devicePixelRatio / HiDPI
   handling, WebGL line-width limits (WebGL often caps width at 1) and how the
   port works around them; colour filters in the web shell.
10. **Frame pacing.** When the screen is redrawn (every frame vs on change).
11. **Pitfalls/lessons.** Bugs, TODOs, comments, or visible artefacts (gaps at
    line joins, shimmer, thickness inconsistency) worth avoiding.
12. **Licence.** Any LICENSE/COPYING/headers in what you read, verbatim name and
    path only.

**Deliverable:** one Markdown report, `port-vector-study.md`, written outside
the preservation repo. Sections: Sources read (repo, commit, files), answers
1–12, and "Recommendations for a SDL3 renderer drawing 256×192 segments at
device resolution" (max 10 bullets, each tagged with the answer it rests on).
Mark anything you inferred rather than read as *inferred*.
