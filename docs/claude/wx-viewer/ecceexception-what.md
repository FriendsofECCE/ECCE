---
type: map
title: "`EcceException::what()` returns storage that lives as long as the exception"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/util/EcceException.H", "src/util/exceptions/EcceException.C"]
issues: []
---
**`EcceException::what()`** now correctly returns a pointer with
exception-object lifetime (fixed from a dangling-stack-string bug) —
if you see garbled `Throw Log:` text anywhere, that fix predates it and
something new is wrong, not a repeat.
