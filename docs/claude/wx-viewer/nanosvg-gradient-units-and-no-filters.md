---
type: pitfall
title: "Tool icons are rendered by NanoSVG: radial gradients need percent units, and there are no filters"
area: wx-viewer
paths: [data/client/pixmaps/svg, tools/screenshots/icons.py]
issues: [210]
---
`wxBitmapBundle::FromSVGFile` uses NanoSVG. A `radialGradient` written with
unitless fractions (`cx=".35" r=".75"`) is read as user units, so the radius
is under one pixel and the shape paints in its last stop colour only; write
`cx="35%" cy="30%" r="75%"`. `filter` (drop shadows) is ignored, so shadows
are a translucent offset copy or a radial gradient fading to opacity 0.
`tools/screenshots/icons.py` renders SVGs through wx at chosen pixel sizes.
