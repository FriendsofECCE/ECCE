---
type: map
title: "not sorted by energy"
area: mo-diagram
section: ""
---
A fragment column is **not sorted by energy** — `placeFragments()`
moves each level to the mean of the orbitals it became. Anything
that walks it in array order and assumes monotonic energy is wrong.
