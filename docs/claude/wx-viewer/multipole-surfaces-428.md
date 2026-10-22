---
type: rule
title: "Quadrupole, octupole and hexadecapole are angular surfaces r = s|f(n)| of the traceless part, blue where f > 0, vermillion where f < 0 (#428)"
area: wx-viewer
paths: [include/util/Multipole.H, src/util/genutil/Multipole.C, src/viz/propsgcommands/AddMultipoleCmd.C, src/apps/builder/QuadPolePanel.C, data/client/config/PropertyPanelDescriptor.xml, tests/multipole/multipole_test.C, tests/apps/builder_overlay_test.py]
issues: [428, 266, 279]
---
Properties > Electric properties > Quadrapole / Octupole / Hexadecapole Moment
are `QuadPolePanel` on QUADPOLE, OCTAPOLE, HEXPOLE. The mesh maths is wx-free
in `util/Multipole` (`multipole_test`); `AddMultipoleCmd` puts one indexed
face set (icosphere level 5, per-vertex colour and normal) under the property
switch.

- **What the parsers store.** Raw traced Cartesian moments, one number per
  index multiset, labelled (`XX XY XZ YY YZ ZZ`, `XXX YYY ZZZ XYY XXY XXZ XZZ
  YZZ YYZ XYZ`, 15 labels for HEXPOLE; case varies, `fromComponents` ignores
  it). Gaussian: all three, D.Angstrom^(k-1) (no unit on the property).
  NWChem: QUADPOLE only, the parser multiplies a.u. by 1.344911 (D.Angstrom)
  but writes `units: au`. ORCA: QUADPOLE only, the "TOT" line in a.u. (e a0^2),
  unscaled, also `units: au`. So units cannot tell NWChem from ORCA: the
  command converts ORCA by the code's name. MOPAC stores no multipoles.
- **Traces.** Codes print the traced tensor; the trace parts are lower
  multipoles times r^2 and would draw a sphere. `harmonicPart` removes the
  spherical-harmonic parts of degree < k (quadrature projection, then a fit
  of the degree-k monomials), so the surface is the pure rank-k multipole.
  The table still shows what the code printed.
- **Origin.** The parsers do not read it. ORCA states "Choice of electric
  origin: Center of mass", so ORCA surfaces are drawn there. NWChem prints
  "Multipole analysis of the density wrt the origin" and Gaussian reports
  about its coordinate origin, so every other code is drawn at (0,0,0) of
  the output's frame. That is the centre of nuclear charge only in
  Gaussian's standard orientation and NWChem's first geometry: NoSymm
  Gaussian runs and later NWChem optimisation steps are not re-centred.
- **Scale.** The slider is the length of the largest lobe in Angstrom (the
  same resolution for every molecule and rank); the default is 1.5 times the
  farthest atom from the origin. The command's Amplitude is Angstrom per
  D.Angstrom^(k-1), shown under the slider.
- **Look.** Surface transparency 0.4, `SORTED_OBJECT_BLEND` while the panel
  has focus (as the polarizability), near faces only (`SOLID`). Colours are
  fixed Okabe-Ito blue #0072B2 / vermillion #D55E00; no colour control.
- **Checks.** Axial quadrupole invariant under rotation about z (two caps,
  one belt); tetrahedral octupole `T_xyz` gives four positive and four
  negative lobes alternating across the coordinate planes; cubic
  hexadecapole is invariant under all 48 Oh operations (six positive caps on
  the axes, the negative part one connected belt with minima on the
  diagonals); the Gaussian water (C2 along y) is unchanged by x -> -x and
  z -> -z for all three; the ORCA h2o output's traceless part matches
  numerically. Builder script `surface PROP` prints vertices, centre and the
  farthest vertex.
