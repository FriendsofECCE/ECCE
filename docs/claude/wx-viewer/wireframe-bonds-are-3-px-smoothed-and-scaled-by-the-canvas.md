---
type: rule
title: "Wireframe bonds are 3 px, smoothed, and multiplied by the canvas content scale (#227)"
area: wx-viewer
paths: [src/tdat/chemistry/DisplayDescriptor.C, src/viz/sgcommands/SGContainer.C, src/inv/moiv/ChemDisplayWireframe.C, src/inv/wxinv/SoWxRenderArea.C, include/inv/ChemKit/ChemDisplayParam.H]
issues: [227]
---
At 1 px the (then default) Ball And Wireframe bonds were unreadable on white; the default style is now Ball And Stick, see the entry on thin sticks.
`DisplayDescriptor::LINEWDTH` is 3 and `SGContainer::applyStyle` sets
`bondWireframeAntiAlias` to WITH_DEPTH_COMPARISON. GL line width is in device
pixels, so `SoWxRenderArea::OnSize` publishes `GetContentScaleFactor()` through
`ChemDisplayParam::setLineWidthScale` and every `glLineWidth` of the bond
wireframe multiplies by it. The scale is process-wide (the last canvas to
resize wins); offscreen renders use whatever it holds. A saved per-style
width is only written when it differs from the default, so a width of 1 saved
before this change is gone and reads back as 3.
