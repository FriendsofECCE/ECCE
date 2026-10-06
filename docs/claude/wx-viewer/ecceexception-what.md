---
type: map
title: "`EcceException::what()` returns storage that lives as long as the exception"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/util/EcceException.H", "src/util/exceptions/EcceException.C"]
issues: []
---
**`EcceException::what()` must return a pointer into the exception
object**, never into a local string: a temporary's buffer dangles once
`what()` returns and shows up as garbled `Throw Log:` text. Garbled text
there now means a new fault, not this one.
