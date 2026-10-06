---
type: pitfall
title: "`SoWxRenderArea::renderCB` during a paint defers the redraw through `p_redrawPending`"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
issues: [99]
paths: ["src/inv/wxinv/SoWxRenderArea.C"]
---
**A `renderCB` while a paint is in flight (`p_inPaint`) must be deferred,
not dropped**: Inventor's sensor has already fired for that change and will not
fire again, so the redraw would be lost. It sets `p_redrawPending`, and
`OnPaint()` repaints once more when it finishes. This path works; a viewer
that stops updating is usually a redraw that was never requested, see
[the redraw-sensor entry](open-inventors-redraw-sensor-is-a-one.md).
