---
type: map
title: "Fragment columns may be opened out for legibility, and so is the molecular column"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["src/apps/builder/MoDiagramCanvas.H"]
issues: [132]
---
Fragment columns may be opened out for legibility, and so is the
molecular column (the canvas spreads it too), so a crowded molecular
level can be drawn a few pixels off its energy. Whatever
computes a correlation line's endpoint must use the same `columnGeometry()`
answer the levels are drawn with, or the lines point at nothing.
