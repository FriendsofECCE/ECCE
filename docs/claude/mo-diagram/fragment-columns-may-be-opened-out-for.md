---
type: map
title: "Fragment columns may be opened out for legibility; the molecular column never is"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["src/apps/builder/MoDiagramCanvas.H"]
issues: [132]
---
Fragment columns may be opened out for legibility; the molecular
column never is, because its energies are the measurement. Whatever
computes a correlation line's endpoint must use the same `columnGeometry()`
answer the levels are drawn with, or the lines point at nothing.
