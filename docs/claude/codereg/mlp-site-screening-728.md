---
type: map
title: "MLP site screening (#728 stage 2C): ecce-sites finds symmetry-distinct voids and vacancy pairs; a series of MLP calculations per site; results as an ensemble"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["src/mlp/sites.hpp", "src/mlp/sites.cpp", "src/mlp/sites_main.cpp", "src/dsm/edsiimpl/Series.C", "tests/mlp/sites_test.py", "help/src/mlp.md"]
issues: [728]
---
- **Symmetry is ECCE's own, not autosym.** autosym/SymmetryOps work on
  molecular point groups; a periodic host needs its space group with the
  centring and supercell translations. `find_symmetry` tries every integer
  rotation part with entries -1/0/1 that keeps the metric (W^T G W = G) and
  every translation that takes one atom of the rarest species onto another,
  and keeps the (W, t) that map all atoms onto atoms of the same element
  within symprec. Each op carries its atom permutation, which is all the
  vacancy-pair classing needs. Fm-3m conventional cell: 192 ops; 2x2x2
  supercell: 1536. A strongly sheared (non-reduced) cell finds fewer ops:
  fewer reductions, never a wrong merge.
- **Voids = local maxima of the nearest-atom distance** on a grid (0.2 Å),
  climbed to the true maximum by a pattern search (the grid alone ranked
  rutile's 4d channel site, 1.82 Å, below two spurious maxima), then moved to the average of its symmetry images that
  fall within max(0.3, 2 grid) of it, three passes: this puts special
  positions exactly on (1/4,1/4,1/4) etc., which the grid alone misses by up
  to a grid diagonal. Candidates within --min-sep of an accepted orbit are
  dropped (largest clearance wins).
- **Environment uses every periodic image within 4 Å, not the minimum
  image**: in a conventional fluorite cell the six Ca at ±a/2 from 4b are
  three atoms seen twice, and a minimum-image count gave CN 3 for Ca.
  Coordination number = neighbours within 1.1 x the nearest distance (1.2
  pulled fluorite's Ca at 2.73 Å into the 2.37 Å F shell).
- **site-NN.xyz comment lines avoid `mult=`/`charge=`**: Series' xyz reader
  takes those words as multiplicity and charge. The comment uses `weight=`.
- **Series reads `Lattice="..."` from an .xyz** (readXyz) and gives the
  Fragment a LatticeDef; before #728 an xyz source was always molecular.
- References used by tests/mlp/sites_test.py are International Tables
  Vol. A: rock salt 8c and fluorite 4b (Fm-3m), rutile 4d (P4_2/mnm), and the fcc pair shells of a
  2x2x2 supercell (192/48/192/48/16 = C(32,2)).
