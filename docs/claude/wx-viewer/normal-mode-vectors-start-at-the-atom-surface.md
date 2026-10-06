---
type: pitfall
title: "Normal-mode arrows start at their atom's surface and are 0.5 A long by default; they stay depth-tested"
area: wx-viewer
paths: [src/viz/propsgcommands/NModeVectCmd.C, src/viz/sgcommands/SGContainer.C, tools/coin/scenes/calc-water-vectors.scene]
issues: []
---
Arrows used to start at the atom centre and disappeared inside the sphere
(an H arrow along the O-H bond ended inside O). `NModeVectCmd` now offsets
the start along the displacement by `SGContainer::getAtomDisplayRadii()`, the
radius ChemDisplay draws that atom with (CPK table, or ball-and-stick radius
times both scale factors; stick uses the bond radius, wire 0). The arrow
length is not shortened by the offset. Arrows are rebuilt only by
`NModeVectCmd`, so a style change needs a new `nmvect` to move the start.
POV-Ray export reads the VRVector transform and gets the offset too.

The auto scale makes the longest arrow 0.5 A (`0.5/maxnorm`); the panel's
Scale slider shows that factor (about 0.7 for water) and a typed value
replaces it. The animation amplitude (`NModeTraceCmd`, `0.25/maxnorm`) is
separate.

Not drawn on top of the molecule, on purpose: depth test off, or an
`SoAnnotation`, paints the far half of each cylinder over the near half so
shafts render black, and a depth-buffer clear looked wrong to the user. An H
arrow that runs along its bond can still lie inside the bond cylinder or the
neighbouring atom and be hidden, most of all in CPK.
Render with `tools/coin/scenes/calc-water-vectors.scene` through
`compare.renderCalc`.
