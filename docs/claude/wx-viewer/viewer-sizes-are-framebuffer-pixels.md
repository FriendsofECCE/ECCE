---
type: pitfall
title: "The Inventor viewer works in framebuffer pixels; wx sizes and mouse positions are logical units (#133)"
area: wx-viewer
paths: [src/inv/wxinv/SoWxRenderArea.C, include/inv/SoWx/SoWxDevice.H, src/inv/wxinv/SoWxMouse.C, src/inv/wxinv/SoWxKeyboard.C, src/inv/wxinv/SoWxExaminerViewer.C, src/wxviz/viewer/SGViewer.C, tests/look/hidpi.py]
issues: [133]
---
On a Retina Mac (and under GDK_SCALE=2 on Linux) a wxGLCanvas's framebuffer
is `GetContentScaleFactor()` times its client size. `SoWxRenderArea::OnSize`
used the client size as Inventor's window size, so the scene was drawn into
the lower-left quarter of a black canvas (glClear still filled all of it).
`getGlxSize()` is now framebuffer pixels, and every wx mouse/key position
goes through `soWxToPixels()` (SoWxDevice.H) before it becomes an SoEvent or
a viewer locator. Any new code that mixes a wx position with `getGlxSize()`,
a viewport region or a pick must convert the same way. wxMSW reports scale 1.
Still pixel-sized, so half as large on Retina: the corner text (`glBitmap`
8x13 font in SGViewer) and the examiner's feedback axes.
Test: `look_hidpi` (viewer-scenes at GDK_SCALE=1/2; `wxpick` sends real wx
mouse events). Water's hydrogens are not hit at their projected centre
after `viewall` by either pick path; the test clicks benzene atoms.
