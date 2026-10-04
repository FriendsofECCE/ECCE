---
type: map
title: "ECCE_USE_COIN (default ON) builds the viewer stack against Coin3D; -DECCE_USE_COIN=OFF keeps the vendored core for one release"
area: wx-viewer
paths: [CMakeLists.txt, tools/coin/gen_shim.py, include/inv/ChemKit/ChemDisplay.H, include/inv/SoWx/SoWxViewer.H, src/inv/wxinv/SoWxRenderArea.C]
issues: [166]
---
`ECCE_USE_COIN` is ON by default (stage 4); with it the build does not compile the vendored
Inventor core in `src/inv` (`db*`, `image`, `interaction`, `nodekits`).
`ecceinv` shrinks to `src/inv/flclient` (our FreeType font layer) plus
`libCoin`; `moiv`, `wxinv`, `src/viz`, `src/wxviz` and the apps build
unchanged otherwise. `<inv/X/Y.H>` includes are forwarded to
`<Inventor/X/Y.h>` by an overlay that `tools/coin/gen_shim.py` writes into
`<build>/coin-shim` at configure time and that is searched before
`include/`; `OIV_COIN` is defined. `inv/ChemKit`, `inv/SoWx` and
`inv/flclient.h` stay ours.

Coin 4.0.10 (EPEL 9, Fedora, Homebrew) installs its headers under
`include/Coin4/Inventor`, not `include/Inventor` as Debian's 4.0.3 and
Ubuntu's 4.0.2 do. `gen_shim.py` once looked only in `/usr/include`, so it
wrote every forwarder as an empty "not in Coin" stub and moiv failed with
`SbString does not name a type`. CMake now `find_path`s the directory that
holds `Inventor/SbString.h` (suffixes Coin4/Coin3/Coin), passes it to
`gen_shim.py` and adds it to the include path when it is not `/usr/include`.

Header fixes are portable and work in both builds: `ChemDisplay.H`
includes Coin's `SoTextureCoordinateElement.h` (a typedef in Coin, so it
cannot be forward-declared) and `SoWxViewer.H`/`SoWxRenderArea.C` include
`SbLinear.h`/`SbColor.h`, which Coin's headers do not pull in transitively.

Why both exist: the vendored core stays buildable (`-DECCE_USE_COIN=OFF`,
CI job "vendored-inventor") for one release as a fallback, then goes. Use
separate build directories (`build-cmake`, `build-oiv`): switching the option
in one tree rebuilds everything. After a change under `src/inv/moiv`,
`src/inv/wxinv` or `include/inv/{ChemKit,SoWx}`, build both.

Packaging: the deb gets `libcoin80t64` from dpkg-shlibdeps (nothing hand
listed); build-dep is `libcoin-dev` plus `libegl-dev` and `python3`
(gen_shim). EPEL 9 and Fedora have `Coin4`/`Coin4-devel` (4.0.10; Fedora also
4.0.7), so the RPM jobs use the Coin build and Requires `Coin4`; those jobs
and the RPMs are unrun as of stage 4.

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

Renderer differences between the libraries that need code in our layer are
listed in the `tools/coin/compare.sh` entry (transparency default, SoCylinder
material, handle-event viewport, stipple leak).
