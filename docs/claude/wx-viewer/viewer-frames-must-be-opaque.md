---
type: pitfall
title: "The viewer's GL canvas has an alpha channel; a frame left at alpha 0 shows what lies beneath it (#999)"
area: wx-viewer
paths: [src/inv/wxinv/SoWxRenderArea.C, src/inv/wxinv/SoWxViewer.C, tests/apps/render_alpha_test.py]
issues: [999]
---
On Coin builds `SoWxViewer` asks for `WX_GL_MIN_ALPHA 8` (depth peeling
needs it), and `SoSceneManager::render` clears with alpha 0, so the
background pixels of every frame are transparent. Opening a large PDB
(1IEP) on a GTK3 X11 desktop with a compositor showed the structure on
black for ~0.4 s although every render's clear colour was the white
Background preference (logged on Xvfb: one `setBackgroundColor`, 44 white
renders). `SoWxRenderArea::redraw()` therefore clears the alpha channel
alone to 1 before the swap (`makeFrameOpaque`).

Xvfb has no compositor and never shows the flash; the check is the frame
alpha itself: `ECCE_TEST_RENDER_CORNER=1` prints a corner pixel of each
frame before and after the fix-up, and `apps_render_alpha` requires every
frame after an import to be opaque white (`ECCE_TEST_IMPORT_SETTLE` keeps
the Builder drawing after the load).
