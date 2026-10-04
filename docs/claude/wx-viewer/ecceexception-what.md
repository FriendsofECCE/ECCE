---
type: map
title: "`EcceException::what()`"
area: wx-viewer
section: ""
---
**`EcceException::what()`** now correctly returns a pointer with
exception-object lifetime (fixed from a dangling-stack-string bug) —
if you see garbled `Throw Log:` text anywhere, that fix predates it and
something new is wrong, not a repeat.
