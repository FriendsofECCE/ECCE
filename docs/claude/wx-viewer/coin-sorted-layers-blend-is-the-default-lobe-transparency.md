---
type: pitfall
title: "Coin SORTED_LAYERS_BLEND is the default lobe transparency (6 passes, alpha canvas); a scene over ~100 ms falls back to SCREEN_DOOR"
area: wx-viewer
paths: [src/inv/wxinv/SoWxRenderArea.C, src/wxviz/viewer/SGViewer.C, src/wxgui/wxdialogs/GlobalPrefs.C, tools/viewer-bench/viewer-bench.C, tools/coin/isoref.py, src/inv/wxinv/SoWxViewer.C, tools/coin/lobes.py, tools/coin/angles.py, src/wxviz/viewer/SceneScript.C]
issues: [166]
---
Two MO lobes (the `ChemIso` positive and negative surfaces) have identical
bounding boxes, so any per-object sort ties; the vendored core and Coin break
the tie in opposite order. Depth peeling is order independent, but:
- Needs an 8-bit alpha in the framebuffer. `SoWxViewer` now asks for it in
  every Coin build (`ECCE_COIN_ALPHA` is gone); if the display cannot supply it
  `SoWxRenderArea::redraw` reads `GL_ALPHA_BITS` on the first frame and uses
  the screen door instead, so Coin never reaches its own "no alpha" warning.
  The EGL thumbnail FBO is RGBA8/D24 and peels fine; llvmpipe supports it.
- Correct where measured (`tools/coin/lobes.py`, depth-tested opaque renders
  as oracle): real MOs 100% of overlap pixels, synthetic pi fields 97.6% at 6
  passes (Coin's default 4: 94.9%; 12+ is worse, not better). The per-object
  sort is 50-55% on both builds.
- Cost: every pass redraws the whole scene. Software GL, 800x800: 10x on small
  molecules (2 -> 26 ms), 6x on the 5184-atom water box (170 -> ~1000 ms).
- Stage 4 made it the default. `SoWxRenderArea::setTransparencyType` maps a
  SCREEN_DOOR request (what Cube/MoPanel send for lobes) to
  SORTED_LAYERS_BLEND with `setSortedLayersNumPasses(6)` unless quick mode is
  on, the canvas has no alpha, or the scene fell back. SORTED_OBJECT_BLEND
  (ESP maps, opaque) and DELAYED_ADD pass through. Vendored build: unchanged.
- Pitfall: the viewer is constructed with SCREEN_DOOR set directly on the
  render action, and `applyTransparency` only runs on a request. Anything
  that shows lobes without MoPanel/Cube focus (the scene script's `mo`
  command, until fixed) stayed on the real screen door. `mo` now sends the
  request; `ECCE_DEBUG_TRANSPARENCY=1` prints each request and the first
  frame's alpha bits (Xvfb gives 8 via the WX_GL_MIN_ALPHA request).
  `LOBE_MODES=builder tools/coin/lobes.py` checks the calc scenes with no
  `transparency` command: 100% nearest-lobe.
- Quick mode: Preferences > General > 3D viewer (`QuickTransparency` in the
  global pref file), or `ECCE_QUICK_TRANSPARENCY=1|0`. `SGViewer::
  setTransparencyType` reads it on every request, so it applies the next time
  an orbital/isosurface panel is shown, not to a scene already on screen.
- Automatic fallback: while peeling, each frame is timed (paint + glFinish);
  the median of the last five over 100 ms switches that viewer to SCREEN_DOOR
  and shows "Large scene: quick transparency" in the top-level frame's status
  bar. One-way until the next setTransparencyType call (panel focus or
  type change). `ECCE_TRANSPARENCY_FALLBACK_MS=<ms>` sets the threshold, 0 disables;
  when it is set the fallback also prints `[TRANSPARENCY] fallback` to stderr.
  Test: `BENCH_FALLBACK=1 build-cmake/viewer-bench` (needs
  `ninja viewer-bench`); on llvmpipe the 5184-atom water box with an
  isosurface falls back and the small molecules do not.
- Pitfall: frames are timed only while peeling is active, and the median of
  five exists because the first frame carries context and shader setup.
- Independent reference (`tools/coin/isoref.py`, `lobes.py --ref`): ray-casts
  exported lobe triangles, composites all hits by depth, no Inventor
  transparency code. Its lighting is assumed (headlight, two-sided) and does
  not match either library (~78% of lobe pixels differ by >24/255), so judge
  by dominant lobe colour in overlap pixels: layers 85%, Coin per-object sort
  57%, vendored 50%. The reference's nearest-lobe agreement is lower than the
  depth-test oracle's 97.6% where lobes interleave in depth.

SCREEN_DOOR (what the Builder uses), measured the same way (`LOBE_MODES=sd
tools/coin/lobes.py`, then `--sd` and `--ref`; real MOs plus synthetic lobes,
0/45/90 deg, alpha 0.5, both builds). Where both lobes' stipple passes in an
overlap pixel (232k px vendored, 205k Coin; the stipple masks of the two lobes
are identical, zero pixels differ), the visible lobe is the nearer one in 100%
of pixels on both builds, and the back lobe shows in 0%: the pixel is front
lobe or background, never a mix. Against `isoref`'s dominant colour on the same
pixels: 96.0% vendored, 95.6% Coin (nearest lobe 99.3% / 99.1%); the reference
is an alpha composite that would show the back lobe at weight 0.25
((1-a)*a), which a screen door cannot do. Counted over all overlap pixels
(stipple holes included) agreement is only 75% / 68%. Vendored and Coin agree on
which lobe is dominant in 100% of pixels both draw, but the stipple density
differs: lobe covers 58.5% of overlap pixels in the vendored core, 51.6% in
Coin at alpha 0.5. So ordering is a non-issue in SCREEN_DOOR.
