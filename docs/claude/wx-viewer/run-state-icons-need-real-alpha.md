---
type: pitfall
title: "Run-state icons in image lists need real alpha: drawn on white and masked by white, they show a white halo on a selected row"
area: wx-viewer
paths: [src/wxgui/wxtools/WxState.C, src/wxgui/wxtools/WxStateImageList.C, src/wxgui/wxtools/WxResourceStateImageList.C, tests/look/stateicons.py]
issues: []
---
The icons are antialiased. Added to a `wxImageList` after drawing on a white
background with white as the mask colour, their edge pixels stay half white, and
a selection colour behind the row shows them as a pale ring (also the amber
"running?" ring). `WxState::alphaBitmap()` draws on a transparent 32-bit bitmap
(`draw(..., clear=false)`), and `WxResourceStateImageList::combineWithState()`
composes the resource icon and the state icon as `wxImage`s with alpha
(`wxIMAGE_ALPHA_BLEND_COMPOSE`); an icon with neither mask nor alpha still has
its white made clear, as the old mask did. Image lists add the bitmaps with no
mask colour. `tests/look/stateicons.py --check` also tests the alpha bitmaps.
