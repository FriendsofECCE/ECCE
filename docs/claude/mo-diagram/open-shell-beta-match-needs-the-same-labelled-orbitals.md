---
type: pitfall
title: "The beta electrons of a merged level are matched by irrep: both spins must be spelled and labelled alike (#301)"
area: mo-diagram
paths: [src/apps/builder/MoDiagramPanel.C]
issues: [301]
---
`matchAlphaBeta` pairs the k-th alpha orbital of an irrep with the k-th beta
orbital of the same irrep. Two ways it silently went wrong, each drawing
only alpha electrons or the wrong beta count:
- a linear molecule's alpha labels were put into the linear spelling
  (`SGG`, `PIU`) but the computed beta labels were not (O2 triplet: no beta
  electron drawn);
- the labeller gave up on a deep near-degenerate a1+t2 set in one spin only
  (CoCl4 2-: Cl 2s-type orbitals), shifting every later t2 match by one set
  (t2* drawn doubly occupied; the electron count then disagreed). An orbital
  labelled in only one spin is now left out of the matching in both.
Test fixtures `orca-o2-triplet`, `orca-cocl4-quartet`.
Charge and spin multiplicity of a fixture reach the Builder as DAV
properties (`.davprops`: `spinmultiplicity` on the calculation, `charge` on
`Parameters/chemsys.mvm`); the `charge:` line inside chemsys.mvm is not
what the Builder reads on this route.
