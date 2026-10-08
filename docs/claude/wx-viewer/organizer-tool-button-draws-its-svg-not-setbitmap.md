---
type: pitfall
title: "Organizer tool buttons paint their SVG icon; setBitMap (the calculation thumbnail) must replace it there"
area: wx-viewer
paths: [src/wxgui/wxtools/EcceTool.C, include/wxgui/EcceTool.H, src/apps/organizer/CalculationContextPanel.C, src/wxviz/viewer/VizRender.C]
issues: [210]
---
`EcceTool::OnPaint` draws `p_bundle` (the #210 SVG icon) when there is one
and `p_bitmap` only otherwise. `CalculationContextPanel` hands the stored
Thumbnail.jpeg to `setBitMap`, which set only `p_bitmap`, so after the SVG
icons no thumbnail was shown on any platform although File > Save stored it
(Parameters/Thumbnail.jpeg, about 1 kB). `setBitMap` now keeps a copy scaled
to the icon size that the paint draws in the icon's place.

The thumbnail itself renders fine headless on Linux (EGL offscreen, see the
`ECCE_USE_COIN` entry); Coin's `glxglue_isdirect()` warning is unrelated.
A missing thumbnail is three separate questions: was it written (Builder
log, `VizRender::msg()`), was it stored whole (binary I/O on Windows, see
wxmsw-silent-differences), was it drawn (this).
