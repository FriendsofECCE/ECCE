---
type: pitfall
title: "Ball and Stick spent most of its frame in the bond-cap test, O(atoms x bonds); the test itself is not a start/count range test"
area: wx-viewer
paths: [src/inv/moiv/ChemUnitCylinder.C, src/inv/moiv/ChemDisplay.C, include/inv/ChemKit/ChemDisplay.H, src/viz/sgcommands/SGContainer.C, tools/viewer-bench/viewer-bench.C]
issues: [224]
---
`ChemUnitCylinder::render`, `renderHalfBonded` and
`renderWithInterpolatedColor` ask, per bond end, whether to draw the
cylinder's end cap (`PRE_RENDER_CAP`). That test scanned every entry of
`ChemDisplay::atomIndex`, and `SGContainer` fills `atomIndex` with one
`(atom, 1)` entry per atom, so a frame cost atoms x bonds field reads:
about 800M instructions per frame for the 5184-atom water box, which is
the gap between Ball and Stick (171 ms on an RTX 3050) and CPK (20 ms).
Now `ChemDisplay::GLRender` reduces `atomIndex` to two bounds once per
frame and `bondCapAtAtom()` answers in O(1).

The old test matches an entry when `count == -1 && atom >= start`, or
when `atom <= start && atom <= count`, which is not "atom lies in
[start, start+count)". With per-atom entries only atoms 0 and 1 lose their
caps; every other cap is drawn inside its sphere. `bondCapAtAtom` keeps
that result exactly; fixing the test would draw less but can change pixels,
so check it with `tools/coin/scenes/styles.scene` (`waterbox` too) first.

The sibling `ADJUST_CYLINDER` in `ChemDisplayCylinders*.C` has the same
range test but breaks on the first non-matching entry, so it is cheap.

Measuring: `BENCH_STYLES="CPK,Ball And Stick,..."` runs only the water
box. On llvmpipe the frame time is dominated by rasterisation and is too
noisy under load to show this; count instructions instead
(`valgrind --tool=callgrind --toggle-collect='ChemDisplay::GLRender*'`).
`perf` is not available to users on Debian (`perf_event_paranoid=3`).
`GALLIUM_NOOP=1` makes the benchmark exit silently.
