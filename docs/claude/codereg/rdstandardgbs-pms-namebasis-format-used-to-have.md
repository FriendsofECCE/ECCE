---
type: checklist
title: "`rdStandardGBS.pm`'s \"NameBasis\" format used to have two undocumented requirements — FIXED in `78bb8d0`, don't \"re-fix\" "
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["*.expt", "/^(\\w+)\\s+library\\s+(\\\".+\\\")$/i", "rdStandardGBS.pm"]
---
**`rdStandardGBS.pm`'s "NameBasis" format used to have two
undocumented requirements — FIXED in `78bb8d0`, don't "re-fix" the
`*.expt` writers for it.** The original PNNL parser matched
`/^basis \"(\w\w) basis\" (\w+) print/` and
`/^(\w+)\s+library\s+(\".+\")$/i`, so the `basis "ao basis" <type>`
line needed a literal trailing `print` keyword and `<atom> library
"<name>"` lines could not be indented — which every existing
`*.expt`'s writer violated, silently translating named-library basis
assignments to nothing with no error. `78bb8d0` relaxed both
(`(\s+print)?`, and `^\s*` on each), so indented lines and a missing
`print` are now both accepted. What *is* still required: the library
line must end immediately after the quoted name (no trailing text),
and the coordinants token must be a bare word (`spherical`/
`cartesian`). Verified against the current file 2026-09-21.
