---
type: pitfall
title: "Coin SORTED_LAYERS_BLEND orders lobes per pixel but needs an alpha visual and costs a full scene pass per layer"
area: wx-viewer
paths: [tools/coin/isoref.py, src/inv/wxinv/SoWxViewer.C, tools/coin/lobes.py, tools/coin/angles.py, src/wxviz/viewer/SceneScript.C]
issues: [166]
---
Two MO lobes (the `ChemIso` positive and negative surfaces) have identical
bounding boxes, so any per-object sort ties; the vendored core and Coin break
the tie in opposite order. Depth peeling is order independent, but:
- Needs an 8-bit alpha in the framebuffer; the Builder's canvas has none
  (`SoWxViewer` passes its own attribute list), and Coin then warns and uses
  SORTED_OBJECTS_BLEND. `ECCE_COIN_ALPHA=1` requests alpha (dev only). The EGL
  thumbnail FBO is RGBA8/D24 and peels fine; llvmpipe supports it.
- Correct where measured (`tools/coin/lobes.py`, depth-tested opaque renders
  as oracle): real MOs 100% of overlap pixels, synthetic pi fields 97.6% at 6
  passes (Coin's default 4: 94.9%; 12+ is worse, not better). The per-object
  sort is 50-55% on both builds.
- Cost: every pass redraws the whole scene. Software GL, 800x800: 10x on small
  molecules (2 -> 26 ms), 6x on the 5184-atom water box (170 -> ~1000 ms).
- ECCE never blends MO lobes: they are SCREEN_DOOR; SORTED_OBJECT_BLEND is
  used only for ESP maps, which are opaque. So the tie only shows in
  `tools/coin/angles.py`, and peeling would be pure cost in the Builder.
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
