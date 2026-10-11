---
type: rule
title: "MO diagram: the tooltip and the info line use MoPanel::compositionText(); column headings are measured and pushed apart or abbreviated"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: [src/apps/builder/MoDiagramCanvas.H, src/apps/builder/MoDiagramPanel.C, src/apps/builder/MoPanel.C, tests/apps/modiagram_panel_test.py, tests/look/modiagramheads.py]
issues: [132]
---
Hovering a molecular level shows a tooltip, and a click writes the same text
under the diagram: the MO number(s), energy, irrep and occupancy, then the
MOs panel's composition line. The canvas asks its owner
(`MoDiagramClickTarget::orbitalComposition`); `MoDiagramPanel` finds the MOs
panel and calls `MoPanel::compositionText()`, which is what the panel's own
line is made from (`shareList()`), so there is one implementation. A degenerate
set is shown as the MEAN of its members' Mulliken shares by atom and shell (how
a pair is split between its partners is arbitrary; the mean is not).

A click, a tooltip and `compositionText()` call `ensureInitialized()` on the MOs
panel: panels are initialised lazily, and an unopened one has no basis or
coefficients (the click used to say "cannot be computed" until an orbital had
been drawn from the panel itself).

The three column headings are measured (`GetTextExtent`) each paint: if the
centre one would meet a neighbour, "Molecular orbitals" becomes "MOs", the
right one is pushed right and kept inside the window, the left one left.
`tests/look/modiagramheads.py` paints the real canvas at 300 to 1100 px and
finds each heading's box from the pixels that change. Builder scene command
`modiagram <name> tip|click <n>` drives a level without synthetic input.
