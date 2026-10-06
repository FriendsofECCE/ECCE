---
type: checklist
title: "Keep `len(TEVEC) <= len(GEOMTRACE)`"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: ["src/apps/builder/GeomTracePropertyPanel.C", "src/tdat/properties/PropTSVecTable.C"]
issues: [86]
---
**Keep `len(TEVEC) <= len(GEOMTRACE)`.** `GeomTracePropertyPanel`
plots any `PropTSVector<Geometry Step>` alongside GEOMTRACE, and
`OnPointClick` passes the curve index straight to `GTStepCmd` — an
index past the last GEOMTRACE frame trips `PropTSVecTable::value()`'s
bounds check and the atoms collapse to the origin. If a code prints a
per-cycle energy trace but not per-cycle geometries, don't map it to
TEVEC.
