---
type: pitfall
title: "Layer builder (#357): spacing, tilt, wrapping and the water-H distance have silent traps"
area: wx-viewer
paths: [include/tdat/LayerStack.H, src/tdat/chemistry/LayerStack.C, src/viz/sgcommands/PBCLayerCmd.C, src/apps/builder/LayerBuilder.C, src/apps/builder/BuilderPbcTest.C, tests/layerstack/layer_stack_test.C]
issues: [357]
---
- Spacing is the perpendicular distance along the a-b normal; shifts are
  cumulative (layer i is moved i*shift). New c = N*d*normal + the in-plane
  part of the old c. A stack drawn from a tilted cell therefore wraps
  differently from a script that writes an orthogonal box: the same atom can
  differ from a reference by a lattice vector of either cell. Compare
  modulo both cells (layer_stack_test does).
- A layer saved folded into the cell straddles the c boundary; its
  thickness must be found from the largest gap around the cell, and only
  then lifted by c. A layer that is already contiguous must not be lifted
  (a lift by the old c is not a vector of the new cell).
- Replicate puts real atoms in the structure, so the command uses
  unit vectors times the replication factors and leaves 1x1x1 (as Super
  does). The command must not auto-replicate on execute or undo.
- Water H are kept 1.5 A from every other atom: closer and
  `addCovalentBonds` perceives a bond to a layer oxygen.
- Wrapping uses `makeMoleculesWhole(true)`, which gives up above 6000 atoms;
  the command then falls back to per-atom folding. The "origin" there is the
  lattice corner, which follows the Center Lattice setting.
- Ion charge is stored as the atom's partial charge: the Builder has no
  separate formal charge.
- The random placement uses its own mapping from mt19937, not
  std::uniform_real_distribution, so a seed gives the same stack on every
  standard library.
- A structure read from a .gro or .pdb has residues, and the Builder's
  residue drawing (`SGFragment::generateProteinAttributes`) dereferences
  every atom's residue: an added ion or water without one crashed Build
  Layers. Each added ion and water gets its own residue (`HOH`, the ion's
  symbol) when the fragment has residues. Scene hook `importfile <path>`
  loads such a file headless (`pbc_layers_test.py` gro-residues).
- The controls are their own tool pane, Tools > Layer Builder
  (`LayerBuilder.C`, #401), not part of the Periodic Builder; the hooks
  `pbclayer` and `pbcpress layers` find them in that pane, open or not.
