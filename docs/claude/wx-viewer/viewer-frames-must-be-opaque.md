---
type: pitfall
title: "The viewer's GL canvas has an alpha channel; a frame left at alpha 0 shows what lies beneath it (#999)"
area: wx-viewer
paths: [src/inv/wxinv/SoWxRenderArea.C, src/inv/wxinv/SoWxViewer.C, src/inv/moiv/ChemDisplay.C, src/inv/moiv/ChemUnitSphere.C, include/inv/ChemKit/ChemUnitSphere.H, tests/apps/render_alpha_test.py]
issues: [999]
---
On Coin builds `SoWxViewer` asks for `WX_GL_MIN_ALPHA 8` (depth peeling
needs it), and `SoSceneManager::render` clears with alpha 0, so the
background pixels of every frame are transparent (measured on Xvfb:
corner alpha 0 in every frame). A compositing X server may then show what
lies beneath the GL window. `SoWxRenderArea::redraw()` clears the alpha
channel alone to 1 before the swap (`makeFrameOpaque`).

A separate first-frame black flash came from
`ChemUnitSphere::generateBillboard`, called by `ChemDisplay::GLRender` even
for ordinary atom styles. It rendered its sphere texture in the current
framebuffer and cleared it to black twice, after the scene manager's white
clear. Interposed `glClear`/`glClearColor`/`glColorMask` showed white, black,
black in the first render, with all colour channels writable; subsequent
renders cleared only to white. Clearing before the scene traversal cannot
fix a clear inside it.

On Coin builds, billboard generation now uses a temporary colour/depth FBO
through Coin's portable GL dispatch and restores framebuffer bindings and GL
state. First-time generation invalidates the enclosing render cache: FBO
bindings execute immediately during display-list compilation, so retaining
the scratch draws would replay them against the window. It must never use
the scene framebuffer as scratch space, including when FBO allocation fails. The vendored-Inventor path retains its old
billboard generation and is not covered by this fix.

`ECCE_TEST_RENDER_CORNER=1` prints each frame's corner before/after the alpha
fix-up with the area, background and size. `apps_render_alpha` imports
1iep.pdb (4710 atoms) with `ECCE_TEST_IMPORT_SETTLE` and checks every frame
from import start, including the first, for opaque white. This checks the
framebuffer before swapping; a real compositing desktop still needs a visual
check for presentation behaviour.
