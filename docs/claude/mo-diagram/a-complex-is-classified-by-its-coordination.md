---
type: map
title: "A complex is classified by its coordination skeleton, not the molecule"
area: mo-diagram
section: ""
---
**A complex is classified by its coordination skeleton, not the
molecule.** Six ammonia rotors cannot all be octahedral, so a
whole-molecule search returns Th at best and C1 in practice. The
panel runs its symmetry search on `MoFragments::coordinationSkeleton()`.
Polyatomic ligands contribute **one sigma donor each**, not their
whole basis; the pi set comes from subtracting sigma from the p-shell
reduction.
