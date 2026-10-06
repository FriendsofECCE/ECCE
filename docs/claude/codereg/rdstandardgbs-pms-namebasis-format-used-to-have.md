---
type: checklist
title: "`rdStandardGBS.pm` accepts indented NameBasis lines and an optional `print`; do not re-fix the `*.expt` writers"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["scripts/parsers/rdStandardGBS.pm", "scripts/parsers"]
issues: [38]
---
**`rdStandardGBS.pm`'s "NameBasis" reader accepts both forms the `*.expt`
writers produce**: the `basis "ao basis" <type>` line with or without a
trailing `print`, and indented `<atom> library "<name>"` lines. The original
parser required `print` and no indent, which every writer violated, so
named-library basis assignments were silently dropped; the reader was
relaxed instead of the writers, so do not change the writers for it. Still
required: the library line ends immediately after the quoted name, and the
coordinates token is a bare word (`spherical`/`cartesian`).
