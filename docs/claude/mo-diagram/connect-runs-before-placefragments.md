---
type: map
title: "connect() runs BEFORE placeFragments()"
area: mo-diagram
section: ""
---
**connect() runs BEFORE placeFragments()**, so fragment levels still
carry tabulated energies in eV while the molecular column is in
Hartree. Anything comparing the two magnitudes directly is wrong —
normalise each column to its own range first.
