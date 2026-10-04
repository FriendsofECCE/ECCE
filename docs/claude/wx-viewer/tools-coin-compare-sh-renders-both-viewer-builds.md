---
type: map
title: "tools/coin/compare.sh renders a fixed scene set with both viewer builds (#166 stage 2); pitfalls of the harness"
area: wx-viewer
paths: [tools/coin/compare.sh, tools/coin/compare.py, tools/coin/viewer-scenes.C, tools/coin/scenes, src/wxviz/viewer/SceneScript.C, include/wxviz/SceneScript.H, src/apps/builder/Builder.C, src/inv/wxinv/SoWxRenderArea.C]
issues: [166]
---
`SceneScript` (wxviz) runs a text script (`style`, `labels`, `select`, `mo`,
`nmvect`, `nmstep`, `gtstep`, `snap`, `thumb`) against an SGViewer. Two
callers: `viewer-scenes` (built-in XYZ systems, EXCLUDE_FROM_ALL target) and
the Builder, when `ECCE_VIEWER_SCENE=<script>` and `ECCE_VIEWER_SCENE_OUT=<dir>`
are set (inert otherwise; it renders in a second 480x480 viewer on the
Builder's scene graph, then closes). `compare.py` drives both on a private
Xvfb and writes `vendored.png/coin.png/diff.png` per scene plus a contact
sheet of pairs that differ after removing the vendored build's constant
colour offset (#83), estimated per pair as a per-channel median.

Traps found building it:
- `snap` reads the canvas with `glReadPixels` through
  `SoWxRenderArea::setFrameCallback` (called before the buffer swap). The
  Builder's own canvas is only as big as the pane layout leaves it (181x671
  headless), hence the second viewer.
- `SoOffscreenRenderer` (the thumbnail path) cannot make a GLX context on
  Coin under Xvfb ("Couldn't create GLX context"), (stage 3a fixed this with an EGL context, see the ECCE_USE_COIN entry). The vendored class
  crashes inside Mesa when destroyed, so the driver keeps one and `_exit`s.
- Atom labels are drawn in the foreground colour (black on the default black
  background until `ForegroundCmd` runs) and need the bundled font; `fl.c` finds
  it from `ECCE_HOME` (see the atom-labels entry; the harness must not set
  `FL_FONT_PATH`, which once hid blank labels on both builds).
- A selected atom shows only if it goes through the viewer's `SGSelection`
  node (`getSelectionPath` + `ChemDisplayPath` + `merge`); filling
  `m_atomHighLight` alone changes nothing on screen.
- The vendored build's offset is about +41 on every channel for lit
  surfaces, not only green; compare after a per-channel correction.
- `IsoValueCmd`'s "Value" is a log10 slider value (10^Value is the isovalue);
  passing 0.05 puts the surface above the field maximum and draws nothing.
- Normal-mode indices are 0-based and the fixture has 3 modes; an index past
  the table gives zero displacement, not an error.
- `SO_NODE_INIT_CLASS`'s third argument is the PARENT's name, looked up
  with `SoType::fromName`. `PropSGFragment` passed its own name, which gave
  a type outside the SoNode tree: Coin aborted on it, the vendored core
  left null action methods (crash in `SoSearchAction`). Fixed; it is now
  registered from `SGContainer::initClass()`, so every app that builds a
  container scene gets it.
- `IsoLib` wrote a leading end-of-strip `-1` in `coordIndex`; Coin takes it
  for an erroneous polygon and draws nothing. Skipped under `OIV_COIN`.
  (`#ifdef __coin` blocks elsewhere in `moiv` are dead: nothing defines it.)
- Stage 3b findings (each one a visible difference, found by diffing GL state
  per frame with `tools/coin/gltrace.c`):
  - Coin's render action starts in BLEND, Open Inventor's in SCREEN_DOOR; MO
    lobes (alpha 0.5) were blended onto black = half brightness. Set in
    `SoWxRenderArea`'s constructor. (Panels asking for SCREEN_DOOR later get
    depth-peeled blending in Coin builds; the scene scripts' `transparency`
    command sets the action directly and bypasses that.)
  - Coin's `SoCylinder::GLRender` sends the state's grey diffuse on every
    call, overwriting `ChemUnitCylinder`'s per-bond `glColor3fv`, and uses 20
    slices where Open Inventor uses 16. The SOCYLINDER branch (the Builder's
    default style) now draws the Open Inventor geometry itself.
  - Coin's `SoSceneManager::setWindowSize` sizes only the GL action; the
    handle-event action stayed 400x400, so `SGSelection`'s ray pick missed
    every atom at 480x480. Real picking in a Coin Builder was broken.
  - Coin leaves `GL_POLYGON_STIPPLE` on after a stippled lobe; `ChemDisplay`
    (glColor, no lazy-element alpha send) then drew atoms half missing.
- Scene commands `pick`, `drag` (SoEvents built in code and sent to the
  scene manager, so SoHandleEventAction -> SGSelection -> processMotion run;
  a listener copies `Builder::motionChanged`), `redraws` (frames per scene
  change, no forced paint), `vizthumb`, `transparency`. Text results are
  compared by `compare.py`. `snap` forces a paint, so it cannot show #99.
- Not defects: Coin's stipple pattern for alpha 0.498 has 16 more set bits
  per 1024 (1.6% more lobe pixels; the ~1650 isolated pixels in mo5);
  SORTED_OBJECT_BLEND orders two equidistant lobes differently (ESP maps are
  one opaque surface); the vendored build shows no normal-mode vectors on the
  first paint after `nmvect` (stale until the next camera change) and Coin
  does, so Coin is right.
