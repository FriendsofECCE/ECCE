---
type: map
title: "Look at `tools/modiagram/render`'s output before believing any layout claim"
area: mo-diagram
section: ""
paths: ["draw.py", "tools/modiagram/render"]
---
**Look at `tools/modiagram/render`'s output before believing any
layout claim.** It runs the real engine and paints the real
`MoDiagramCanvas` to a PNG in seconds. `draw.py` is a *second*
implementation of the picture and drifted far enough that the
diagram looked right in the tool and wrong in builder for hours.
