---
type: pitfall
title: "The Geometry Trace panel asked for a property named \"\" when nothing can be plotted"
area: wx-viewer
paths: ["src/apps/builder/GeomTracePropertyPanel.C", "scripts/parsers/qe.desc"]
issues: []
---
**`GeomTracePropertyPanel::fillPlot()` fetched the property `""` when the
calculation stores a trace but no per-step series.** Quantum ESPRESSO stores
`GEOMTRACE` and no `TEVEC`/gradient series, so `p_currentProp` stayed empty;
`getProperty("")` fetches the whole `Props/` collection, the data server
answered `301 Moved Permanently` with an HTML page, and `PropertyDoc::parse`
crashed (SIGSEGV when the panel was first shown, found with gdb on a
relax calculation of the QE tutorial). `fillPlot` now returns for an empty
name; the panel then has the frames to play and an empty plot. A per-step
energy series for QE (TEVEC) is not stored: pw.x prints the energy of the
starting geometry first and `qe.geomtrace` starts at the first move, so the
two are offset by one and a stateless `.desc` entry cannot pair them (see
`qe.desc`).
