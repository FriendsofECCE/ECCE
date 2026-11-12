---
type: pitfall
title: "CRYST1 made a docking receptor periodic: the cell is drawn around the molecules and every viewAll frames it (#968)"
area: wx-viewer
paths:
  - src/tdat/chemistry/Fragment.C
  - include/tdat/Fragment.H
  - src/apps/builder/PdbCalculation.C
  - src/apps/builder/EnsemblePropertyPanel.C
  - tests/launch/launchjob.C
issues: [968, 930]
---
Every crystallographic entry carries a `CRYST1` record, and
`Fragment::restorePDB` took it as the cell of a periodic system (#930, which
needed it for a GROMACS box). For a docking receptor the cell is meaningless
— receptor and ligand are one finite system — and it is not merely cosmetic:
the lattice is stored with the structure, `Builder::setContext` opens the
Periodic Builder and draws it for any fragment that has one, the drawn cell
is a sibling of the structure in the scene graph so every `viewAll()` frames
it (the molecules become a speck), and a lattice also makes the Builder draw
boundary atoms again as cell images, which every scene-graph command then
walks.

`restorePDB` takes `useCell` (default `true`, so the GROMACS and MD readers
are unchanged). The caller that knows the structure is finite passes `false`:
the Builder's PDB open (`PdbCalculation.C`) does, and so must any harness
that stands in for it (`tests/launch/launchjob.C` reads a receptor the same
way). Gating the read, not the drawing, is one place and keeps the cell out
of the stored structure as well as out of the view.

`EnsemblePropertyPanel::fitPocketCamera` was never the problem: it frames the
shown atom coordinates and the drawn search box only, and measured 0.1 ms and
a 45 A framing on 1IEP. What leaves the pocket a speck is a `viewAll()` on a
scene that contains the cell.
