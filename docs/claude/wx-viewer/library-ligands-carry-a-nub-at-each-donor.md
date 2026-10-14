---
type: map
title: "Structure Library ligands: a Nub atom bonded to each donor atom, pointing at the metal site"
area: wx-viewer
paths: ["data/client/StructureLibrary/Ligands", "src/viz/sgcommands/SGFragment.C", "src/apps/builder/StructLib.C"]
issues: []
---
A library fragment attaches through its `Nub` pseudo-atoms (symbol `Nub`, one
bond to the parent atom, as in `Amino_Acids/Interior`). In Add Structure mode
the Builder fuses the fragment's selected nub (the only nub, if there is just
one: `StructLib::getSelectedAtom`) to the nub selected in the molecule, and
`SGFragment::bondingAtoms` bonds the two parents; a metal added with a shape
(octahedral, ...) has the nubs to receive it. The nub's distance from its
parent is reset by `fixAllNubs`, only its direction matters.

`Ligands/*.mvm` put one nub on each donor atom. A monodentate ligand's nub
points along the lone pair (away from the donor's neighbours); a chelate's
nubs both point at the metal position on the ligand's symmetry axis (2.0 A
from the donors), so after attaching through one donor the other is closed
with Bond on the two nubs. Cyclopentadienyl, a π ligand, has no nubs: it is bonded with the Metal
order, and a nub left on an unbonded ring carbon (η3) made Clean bend its H. Geometries are MOPAC PM7 (anions with the
charge; `charge:` is not read by the .mvm reader, so the charge is only in the
comment line); 2,2'-bipyridine is the rigidly rotated cis conformer.
