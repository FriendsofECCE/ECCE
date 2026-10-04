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
