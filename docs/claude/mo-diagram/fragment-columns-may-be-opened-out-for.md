---
type: map
title: "Columns are opened out vertically for legibility, never sideways"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["src/apps/builder/MoDiagramCanvas.H"]
issues: [132]
---
Fragment, molecular and calculation columns share `MoDiagramCanvas::openRows`:
every level gets its own row in energy order, at least a label height apart,
with no horizontal offset (a level drawn beside another reads as a separate
group). A row can therefore be a few pixels off its energy. Whatever computes a
correlation line's endpoint must use the same `columnGeometry()` /
`p_cLanes` answer the levels are drawn with, or the lines point at nothing.

A row carrying an oblique (Oh/Td) phase sketch needs `2*sketchHalfHeight()`
pixels, not the 46 of a flat one; `centreOverlaps()` counts sketch boxes of the
fragment rows too (`p_fragSketch`).
