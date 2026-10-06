---
type: map
title: "Look at `tools/modiagram/render`'s output before believing any layout claim"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["tools/modiagram/draw.py", "tools/modiagram/render.C"]
issues: [132]
---
**Look at `tools/modiagram/render`'s output before believing any
layout claim.** It runs the real engine and paints the real
`MoDiagramCanvas` to a PNG in seconds. `draw.py` is a *second*
implementation of the picture and drifted far enough that the
diagram looked right in the tool and wrong in builder for hours.
