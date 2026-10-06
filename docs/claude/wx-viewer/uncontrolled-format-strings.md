---
type: map
title: "Uncontrolled format strings"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: []
issues: []
---
**Uncontrolled format strings**: `wxLogError(msg.c_str(), 0)` treats
dynamic text as a printf format — fixed 9 sites codebase-wide, pattern
was always `wxLogError("%s", msg.c_str())`.
