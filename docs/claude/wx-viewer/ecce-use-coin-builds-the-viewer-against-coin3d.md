---
type: map
title: "ECCE_USE_COIN builds the viewer stack against Coin3D; both builds coexist until stage 4"
area: wx-viewer
paths: [CMakeLists.txt, tools/coin/gen_shim.py, include/inv/ChemKit/ChemDisplay.H, include/inv/SoWx/SoWxViewer.H, src/inv/wxinv/SoWxRenderArea.C]
issues: [166]
---
`cmake -DECCE_USE_COIN=ON` (default OFF) does not compile the vendored
Inventor core in `src/inv` (`db*`, `image`, `interaction`, `nodekits`).
`ecceinv` shrinks to `src/inv/flclient` (our FreeType font layer) plus
`libCoin`; `moiv`, `wxinv`, `src/viz`, `src/wxviz` and the apps build
unchanged otherwise. `<inv/X/Y.H>` includes are forwarded to
`<Inventor/X/Y.h>` by an overlay that `tools/coin/gen_shim.py` writes into
`<build>/coin-shim` at configure time and that is searched before
`include/`; `OIV_COIN` is defined. `inv/ChemKit`, `inv/SoWx` and
`inv/flclient.h` stay ours.

Header fixes are portable and work in both builds: `ChemDisplay.H`
includes Coin's `SoTextureCoordinateElement.h` (a typedef in Coin, so it
cannot be forward-declared) and `SoWxViewer.H`/`SoWxRenderArea.C` include
`SbLinear.h`/`SbColor.h`, which Coin's headers do not pull in transitively.

Why both exist: the vendored core is still the shipped, tested viewer; the
Coin build is validated side by side (stage 2 onwards) and the vendored
core is removed only at stage 4. Use separate build directories
(`build-cmake`, `build-coin`): switching the option in one tree rebuilds
everything. After a change under `src/inv/moiv`, `src/inv/wxinv` or
`include/inv/{ChemKit,SoWx}`, build both.

Thumbnails (`SoOffscreenRenderer`): Coin's own GLX offscreen context fails
under Xvfb and crashes with no `DISPLAY`. `src/inv/wxinv/CoinEglOffscreen.C`
(installed from `SoWx::init`, Coin builds on Linux only) supplies a Mesa EGL
context + FBO through `cc_glglue_context_set_offscreen_cb_functions`. Traps:
- Coin asserts a GLX context is current when it first sees a context; the
  check cannot see EGL, so `COIN_GL_NO_CURRENT_CONTEXT_CHECK` is set in code.
- glvnd refuses `eglMakeCurrent` (`EGL_BAD_ACCESS`) while a GLX context, such
  as the wx canvas's, is current: release it first and give it back.
- Coin casts our callback handle to its own GLX context struct in
  `cc_glglue_context_max_dimensions` (reads +0x40/+0x48/+0x50); the handle
  starts with zeroed padding so that path answers "no pbuffer".
- Coin prints one harmless `glxglue_isdirect()` warning per run.
`make offscreen-check` (target, Coin build) renders with no wx and no display.
