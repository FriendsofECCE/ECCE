---
type: pitfall
title: "The viewer's GL canvas has an alpha channel; a frame left at alpha 0 shows what lies beneath it (#999)"
area: wx-viewer
paths: [src/inv/wxinv/SoWxRenderArea.C, src/inv/wxinv/SoWxViewer.C, tests/apps/render_alpha_test.py]
issues: [999]
---
On Coin builds `SoWxViewer` asks for `WX_GL_MIN_ALPHA 8` (depth peeling
needs it), and `SoSceneManager::render` clears with alpha 0, so the
background pixels of every frame are transparent (measured on Xvfb:
corner alpha 0 in every frame). A compositing X server may then show what
lies beneath the GL window. `SoWxRenderArea::redraw()` clears the alpha
channel alone to 1 before the swap (`makeFrameOpaque`).

Still open: on Xvfb, the first frame after importing 1iep.pdb has a black
corner (0,0,0, alpha 0) while the scene manager's background is white; the
next eight frames are white. A glClear to the background before
`actualRedraw()` did not change it, so something inside that render writes
black. `ECCE_TEST_RENDER_CORNER=1` prints each frame's corner before/after
the fix-up with the area, its background and size; `apps_render_alpha`
(import with `ECCE_TEST_IMPORT_SETTLE`) fails on that frame.
