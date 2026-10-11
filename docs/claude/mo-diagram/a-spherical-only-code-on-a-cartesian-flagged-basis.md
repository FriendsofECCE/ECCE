---
type: pitfall
title: "A spherical-only code on a basis recorded as Cartesian: every column read unpermuted, no labels (ECCE-QM)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["src/tdat/chemistry/MoAoOrder.C", "src/apps/builder/MoDiagramPanel.C", "tests/modiagram/makecalc.py"]
issues: [132]
---
ECCE-QM (and ORCA) run Pople sets with five d functions, but a basis
stored with coordsys Cartesian (the library's Pople sets, or a calculation
made before `GBSRules::enforceCodeCoordSys()`) made
`MoAoOrder::reorderToNative()` count six, miss the table's width and
return the coefficients UNPERMUTED. The diagram then computed labels from
columns in the wrong order: c.S.c of 0.4 to 58, no label accepted
("full-group labels not computed (step 132)"), a correct point group (D4h)
and fragment columns with no TASOs, as if C1. The rest of the panel had
already tried both coordinate systems by width; this was the one place
that did not. It now does the same.

Not the cause: the frame. ECCE-QM keeps the input orientation (bond along
x, or arbitrary), while the group's operations are in autosym's frame;
`SymmetryAnalysis::alignFrames()` rotates the operations into the
coefficients' frame and works for any orientation (fixture
`ecceqm-co-rotated`).

Fixtures `ecceqm-*` come from `makecalc.py ecceqm` (real engine run); their
stored code, basis kind and size are set over WebDAV by `capture.py`
from `.davprops`, because the copied property database is ORCA's. A
fixture with the wrong code or coordsys fails the same way, silently.
He2 had no fragment levels until the valence-energy table got He (1s).
