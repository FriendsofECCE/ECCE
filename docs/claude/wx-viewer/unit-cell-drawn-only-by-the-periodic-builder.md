---
type: pitfall
title: "The unit cell is drawn only while the Periodic Builder panel is open; a loaded lattice alone shows nothing"
area: wx-viewer
paths: [src/viz/sgcommands/SGContainer.C, src/apps/builder/PBC.C, src/tdat/chemistry/Fragment.C]
issues: []
---
`Fragment::restoreCAR` (PBC=ON) sets the fragment's `LatticeDef`,
and `SGLattice` would draw it, but `SGContainer` creates its parent
`latticeswitch` as `SO_SWITCH_NONE` and the only code that turns it on is
`PBC::showLattice()`, from `PBC::refresh()`. That runs when the Periodic
Builder panel is opened (Tools > Periodic Builder, via
`Builder::OnToolMenuClick`) and on scene events while it is shown, and
follows its "Show Lattice" check box (checked by default). So opening a
periodic `.car` shows the atoms and no cell until that panel is opened.
This is the original design (`PBC::refresh` avoids stashing fragments while
the toolkit is irrelevant), not a lost event. Closing the panel leaves the
switch as it was.
