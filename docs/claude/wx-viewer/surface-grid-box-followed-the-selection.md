---
type: pitfall
title: "The MOs panel's surface grid covered only the selected atoms (getBoundBox follows the selection)"
area: wx-viewer
paths: ["src/apps/builder/MoPanel.C", "src/viz/sgcommands/SGFragment.C", "src/viz/propsgcommands/ComputeMoCmd.C", "tests/apps/surface_nuclei_test.py"]
issues: [892]
---
`SGFragment::getBoundBox()` is the box of the **selected** atoms when any
are selected (and always includes atom 1); it exists for zoom-to-selection.
`MoPanel::updateGridDlg()` used it for the Density/MO/spin-density grid, so
with a few atoms picked the others lay outside the grid: no density on
them whatever the isovalue or coefficient cutoff, and a surface cut flat
at the box faces. It looked like a basis-mapping fault (#892) but the
per-atom walk in `ComputeMoCmd` is the same walk as
`BasisFlatten::flatten`, from which results.h5 is written. The grid box is
now taken from every atom, plus 4 A. Any new grid for a whole-molecule view: do not
use `getBoundBox()`.

A box that is too small cuts a diffuse orbital flat at the faces (N2
sigma 2s, Li2 localised sigma): after a surface is drawn `MoPanel` checks
|field| on the six faces against the isovalue drawn and grows the cut sides
by 2 A (3 rounds, 40 A edge); the slider moved below the checked value
repeats it. `tests/apps/surface_faces_test.py` covers it.

Since #892 `ComputeMoCmd` fills Density, Spin Density and MO grids from
`DensityPlane::Evaluator` when it reads a checked results.h5
(`ECCE_SURFACE_PROPS=1` forces the old walk); the walk stays for
calculations without one and for localised orbitals.
`tests/apps/surface_nuclei_test.py` compares the grid at every nucleus
with `ecce-density-field --at` (scene command `gridnuclei`).

The growth step of the face check is not a fixed 2 A: it is
`ln(faceMax/iso) + 1` A per cut side (2..8 A), up to 6 rounds, so a low
isovalue on a many-atom cluster closes in a few recomputes. Field views
(ELF/LOL/NCI) are not face-checked. Scene command `moiso <log10 iso>`
moves the slider; `surface_faces_test.py` has cluster density cases.
