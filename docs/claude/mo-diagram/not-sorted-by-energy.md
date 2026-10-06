---
type: map
title: "A fragment column is not sorted by energy"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["src/tdat/chemistry/MoDiagram.C"]
issues: [132]
---
A fragment column is **not sorted by energy** — `placeFragments()`
moves each level to the mean of the orbitals it became. Anything
that walks it in array order and assumes monotonic energy is wrong.
