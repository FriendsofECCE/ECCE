---
type: pitfall
title: "A GROMACS dodecahedron or truncated octahedron is drawn as its Wigner-Seitz cell around (a+b+c)/2, not as the box-vector parallelepiped"
area: wx-viewer
paths: [include/tdat/BoxShape.H, src/tdat/chemistry/BoxShape.C, src/viz/atomnodes/SGLattice.C, tests/boxshape]
issues: [930]
---
GROMACS fills the compact cell centred on the box centre, so the parallelepiped
of the vectors looks wonky with water "outside". `SGLattice::setLattice`
recognises the reduced GROMACS forms (tolerance 0.3 % of |a|) and replaces the
edges; coordinates are untouched. The three axis-coloured first lines are kept
as empty slots because `updateColors` colours vertices 0-5 by position.
`box_shape_test` checks `gmx trjconv -pbc atom -ur compact` fixtures.
