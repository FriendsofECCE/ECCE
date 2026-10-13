---
type: map
title: "Fragment-orbital analysis expands MOs with G^-1, not just C'Sc (#201)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["include/tdat/FragmentAnalysis.H", "include/tdat/FragmentLoader.H", "src/apps/fragments/fragments.C", "tests/fragments"]
issues: [201]
---
`ecce-fragments` expands a parent MO in fragment orbitals as
a = G^-1 T'S c, G = T'ST the overlap of ALL fragment orbitals. Fragments
overlap each other, so the bare T'S c of the issue text is not the expansion;
Multiwfn's CDA solves the same system (`invmatsub` in `CDA.f90`), and the
numbers agree to its printed 0.01 point. Mulliken weight a_j (G a)_j sums to
c'Sc only when the fragment orbitals span the parent basis, which is what the
"sums to 1" check tests for same-basis fragments.

The library takes plain matrices. A fragment from another source (ECCE-QM in
the parent basis already fits; extended Hueckel in its own minimal STO basis
would not) supplies `crossOverlap` S(parent AO, own AO) and `sameBasis=false`;
`FragmentAnalysis::embed()` is the one place that must then project through
S^-1 (not implemented), and the sum becomes a reported capture, not a check.

Only restricted calculations; atoms are matched by position (1e-4 Angstrom)
and a fragment's basis on an atom must equal the parent's. Fixture orbitals
come from ORCA's 6-decimal MO print: the orthonormality check limit is 1e-5,
not 1e-6. Degenerate MOs differ between runs, so fixtures and the Multiwfn
oracle must come from the same ORCA runs.
