---
type: map
title: "MO composition: MoAoBasis, Mulliken by default (#161)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["include/tdat/MoAoBasis.H", "src/tdat/chemistry/MoAoBasis.C", "src/apps/mocomp/mocomp.C", "src/apps/builder/MoPanel.C", "tests/mocomp"]
issues: [161, 132, 927]
---
`MoAoBasis` (tdat, no wx) owns the walk that says what each MO coefficient
column is: atom, l, nth shell of that l, component, and the real overlap
matrix. `MoDiagramPanel`'s `functionsPerAtom()`/`buildBasisOverlap()` are thin
wrappers on it, so the diagram, the MOs panel text, View Coeff's `%` column and
`ecce-mocomp` cannot disagree about the basis.

Default method is Mulliken `c_i (S c)_i / c.S.c` (the diagram's own share);
`c^2` is the option. Mulliken parts are unbounded: do not threshold on them
(that is what the Lowdin share in `MoComposition` is for).

Checked against Multiwfn (Mulliken and Ros-Schuit) for ORCA water, benzene,
CH3 (UHF), Cr(CO)6 and Gaussian 16 formaldehyde with Cartesian d: worst
difference about 0.001 percentage point. `tests/mocomp/README.md` says how the
fixtures and references were made. The fixtures are local-data calculation
folders (`.ecce-meta`), not the data-server `.DAV` folders of `tests/modiagram`,
which `ecce-mocomp` cannot open.

Headless screenshots of the Builder need `ECCE_PANEL_MODE=classic`: in the
default `detail` mode the molecule and the MOs pane did not appear in a
private-Xvfb run, with or without this feature.

Unresolved #927: `mopac.mgf` already applies GRAPHF's inverse square root of
the overlap to store real Slater-AO coefficients. `MoAoBasis::build` instead
treats the Slater-model flag as an identity metric. For the getting-started
ammonia example, stored MO 5 has c.c = 2.25019454, but c.S.c = 1.00000000
using GRAPHF's exact matrix; all seven stored orbitals have c.S.c within
2e-8 of 1. The raw GRAPHF vectors have c.c = 1. ECCE-QM's NH3/6-31G*
example has all 20 c.S.c within 2e-7 of 1 using independently integrated
Gaussian overlaps. Neither example needs the #921 code-basis projection.

Changing only the identity-metric bypass is insufficient: GRAPHF's atom
coordinates are in the FORCE frame, whereas the stored GEOMTRACE is in the
original frame. With the existing stored geometry, real AO overlaps give
MOPAC norms 0.9814, 0.5868, 0.6622, 0.7965, 1.4869, 2.6324, 2.0622.
The metric-only change was withdrawn; coordinate-frame handling must also
be resolved. `tests/mocomp/nh3_norms_test.py` preserves the independent check
and failing production reproducer; it is not registered in the passing suite.
