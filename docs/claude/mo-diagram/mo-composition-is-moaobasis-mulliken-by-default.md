---
type: map
title: "MO composition: MoAoBasis, Mulliken by default (#161)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["include/tdat/MoAoBasis.H", "src/tdat/chemistry/MoAoBasis.C", "src/apps/mocomp/mocomp.C", "src/apps/builder/MoPanel.C", "tests/mocomp"]
issues: [161, 132]
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
