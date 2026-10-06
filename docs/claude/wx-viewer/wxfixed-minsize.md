---
type: map
title: "`wxFIXED_MINSIZE`"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: []
issues: []
---
**`wxFIXED_MINSIZE`** on a sizer item freezes it at whatever best-size
it had *at the moment it was added* — a bug only when an empty
placeholder is added first and filled with real content later.
