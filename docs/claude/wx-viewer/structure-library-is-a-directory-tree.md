---
type: map
title: "The Structure Library is data/client/StructureLibrary: each top-level directory is a Libraries drop-down entry"
area: wx-viewer
paths: ["data/client/StructureLibrary", "src/apps/builder/StructLib.C", "src/apps/builder/BuilderScript.C", "tests/structlib/diatomics.C", "tests/qm/diatomic_order_test.py"]
issues: []
---
A library is a directory of `.mvm` files (`#` starts a comment); the top-level
directories fill the Libraries drop-down, sub-directories are folders in the
list. The reader keeps atoms and bonds only: `charge:` is not read and there is
no spin multiplicity, so a triplet (O2) is set in the Calculation Editor.
Files with no bonds (`num_bonds: 0`) are fine. Lay a diatomic along x: the
preview looks down z and shows one atom if the bond is along z.
`Teaching/Diatomics` holds the first-year-lab molecules at experimental r_e;
`structlib_diatomics` checks the stored distances and `qm_diatomic_order` the
ECCE-QM valence orbital order at them. Headless: `ECCE_BUILDER_SCRIPT` command
`library Teaching/Diatomics CO`.
