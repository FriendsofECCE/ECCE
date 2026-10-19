---
type: pitfall
title: "Labels in a pick's subgroup: a degenerate level is split by projection, and the code's own labels cannot check them"
area: mo-diagram
paths: ["src/apps/builder/MoDiagramPanel.C", "src/tdat/chemistry/SymmetryAnalysis.C", "src/tdat/chemistry/MoFragments.C"]
issues: [301]
---
Labelling the orbitals of CoCl4 in C3v (the subgroup a Co-Cl cut keeps) needs two
things the full-group labelling did not:

- the t2 set (three degenerate orbitals) is a1 + e in C3v. `fullOrbitalIrrep`
  declines a reducible set and its one-by-one fallback labels nothing of it, so
  every t2 level stayed unlabelled and the diagram fell back to ORCA's D2.
  `SymmetryAnalysis::fullOrbitalSplit` reduces the set's character (rounded to the
  whole numbers the tables hold, the orbitals being a thousandth off) and projects
  it on each irrep; the rotated, S-orthonormal combinations replace the panel's
  `moCoefficients` rows (not the stored MO table: a click on such a level shows
  the stored orbital of the set);
- the cross-check against the code's labels (`subgroupLabelSpectrum`) needs the
  code's abelian subgroup to be a subgroup of the group; D2 is not one of C3v. The
  orbitals are therefore labelled in the parent group first (validated against the
  code as usual) and every degenerate level of it must have come out as exactly
  the irreps it subduces to (characters of the subgroup's operations carried into
  the parent's classes through the frame alignment): t2 in Td gives a1 + e in C3v.
  A disagreement, or a parent that cannot be labelled, refuses the subgroup labels
  with the reason as a tier-B line.

The subgroup comes from `MoFragments::subgroupKeeping`: the operations that keep
both sides, named by their axes and mirrors (cubic and rare dihedral subgroups are
refused with a message), and the coordinates are put in that group's standard
frame by scanning the angle about the principal axis until the standard operations
map atoms onto atoms, then symmetrising.
