---
type: map
title: "Uncontrolled format strings"
area: wx-viewer
section: ""
---
**Uncontrolled format strings**: `wxLogError(msg.c_str(), 0)` treats
dynamic text as a printf format — fixed 9 sites codebase-wide, pattern
was always `wxLogError("%s", msg.c_str())`.
