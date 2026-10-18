---
type: pitfall
title: "The unit cell is drawn by the `latticeswitch` node: a loaded lattice turns it on, and closing or unticking the Periodic Builder turns it off"
area: wx-viewer
paths: [src/viz/sgcommands/SGContainer.C, src/apps/builder/PBC.C, src/apps/builder/Builder.C, src/tdat/chemistry/Fragment.C]
issues: [930]
---
`SGContainer` creates `latticeswitch` as `SO_SWITCH_NONE`; `PBC::showLattice`
is the only code that changes it. `Builder::setContext` switches it on for a
structure that carries a lattice (a Prepare's `chemsys.pdb`, a periodic `.car`)
without opening the Periodic Builder; an import opens the panel as well.
`OnToolMenuClick` and `OnPaneClose` switch it off when the Periodic Builder
is unticked or closed. While the panel is shown, `PBC::refresh` follows its
"Show Lattice" box. Test hook: `fragdump` prints `latticeshown`; tests
`tests/apps/pbc_layers_test.py` (box-hides) and `tests/launch/gromacs_prepare_test.py`.
