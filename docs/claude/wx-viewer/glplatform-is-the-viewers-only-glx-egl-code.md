---
type: map
title: "GlPlatform is the viewer's only GLX/EGL code; tools/nonx11/check.py compiles the viewer as a non-X11 port"
area: wx-viewer
paths: [include/inv/SoWx/GlPlatform.H, src/inv/wxinv/GlPlatform.C, src/inv/wxinv/CoinEglOffscreen.C, src/inv/wxinv/SoWx.C, include/inv/SoWx/SoWxViewer.H, tools/nonx11/check.py, tools/nonx11/glcanvas_nonx11.h]
issues: [232, 133, 166]
---
`GlPlatform` (#232 step 4) holds the window-system side of the viewer's GL:
the current-context query/release/restore (GLX) and `installOffscreen()`
(Coin's `SoOffscreenRenderer` on the EGL context of `CoinEglOffscreen.C`).
`ECCE_GL_X11` selects it: 1 on Linux/FreeBSD, 0 elsewhere, where the
context calls are no-ops and `installOffscreen()` leaves Coin's own CGL
(macOS) or WGL (Windows) offscreen path in place. `CoinEglOffscreen.C`
compiles to nothing when `ECCE_GL_X11` is 0. On-screen drawing was already
portable: `SoWxRenderArea` is a `wxGLCanvas` with a `wxGLContext`.

What else in the Coin path touched X11, before step 4 (found by
preprocessing each file with the build's own flags, not by grep; most grep
hits are in `/* */` blocks or `#ifdef __sgi`/`__sys_fonts`):
- `SoWxViewer.H` included `<X11/Intrinsic.h>` only for the types of four
  dead Xt-era methods (`copyView`, `pasteView`, `setNormalVisual`,
  `arrowKeyPressed`); they are gone. `MoPanel.C` had been getting X11's
  `False` through that include.
- `SoWxViewer.C` ended the wx GL attribute list with X11's `None` (0).
- `sysfonts.C` compiles to nothing: `__sys_fonts` is never defined; labels
  come from the FreeType layer (`flclient`).
- Stereo: no live code in either build (the `XSGI*` calls were SGI only), so
  `GlPlatform` has no stereo query.
- `wx/glcanvas.h` on wxGTK pulls in GLX itself; that is wx's, not ours.

`tools/nonx11/check.py [build]` compiles every source of eccewxinv, eccemoiv,
eccevizsg and eccewxviz with `ECCE_GL_X11=0`, `#error` stubs for X11/GLX/EGL
headers and the GTK GL canvas header swapped for the portable API. It is an
approximation (the rest of wx stays wxGTK) and needs a Coin build tree; run
it after adding an include or a GL call to the viewer.
