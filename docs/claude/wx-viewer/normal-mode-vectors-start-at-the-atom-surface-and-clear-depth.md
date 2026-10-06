---
type: pitfall
title: "Normal-mode arrows start at their atom's surface and are drawn after a depth clear; depth test off makes cylinders dark"
area: wx-viewer
paths: [src/viz/propsgcommands/NModeVectCmd.C, src/viz/sgcommands/SGContainer.C, tools/coin/scenes/calc-water-vectors.scene]
issues: []
---
Arrows used to start at the atom centre and disappeared inside the sphere
(an H arrow along the O-H bond ended inside O). `NModeVectCmd` now offsets
the start along the displacement by `SGContainer::getAtomDisplayRadii()`, the
radius ChemDisplay draws that atom with (CPK table, or ball-and-stick radius
times both scale factors; stick uses the bond radius, wire 0). The arrow
length is unchanged. The arrows are rebuilt only by `NModeVectCmd`, so a style
change needs a new `nmvect` to move the start points. POV-Ray export reads the
VRVector transform, so it gets the offset; it has no overlay, so a sphere in
front still hides an arrow there.

On-top drawing is a depth-buffer clear (SoCallback) before the arrow
switch, not `SoDepthBuffer`/`SoAnnotation`: with the depth test off the far
half of each cylinder is painted over the near half and the shaft shows black.
`glClear` works in both the Coin and the vendored build. Look at it with
`tools/coin/scenes/calc-water-vectors.scene` through `compare.renderCalc`.
