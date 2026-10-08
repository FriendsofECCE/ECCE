---
type: pitfall
title: "The MO Diagram is built when a calculation opens, not when its pane is shown"
area: wx-viewer
paths: ["src/apps/builder/MoDiagramPanel.C", "src/apps/builder/PropertyPanel.C", "src/apps/builder/Builder.C"]
issues: []
---
`PropertyPanel::Show(true)` runs `ensureInitialized()`, meant to defer a
panel's work until the user opens it. The MO Diagram prefers floating
(`prefersFloating`), and wxAUI creates the floating frame of a pane at the
first `Update()` even when the pane is hidden: `wxAuiFloatingFrame::
SetPaneWindow` adds the window to the frame's own manager as a shown
centre pane, whose `Update()` calls `window->Show(true)`. So the whole
diagram (point group via autosym, symops, overlap integrals, Lowdin) is
computed for every calculation with orbital energies as the Builder opens
it, with nobody looking; on GTK too. On Windows its child processes used to
flash console windows (see wxmsw-silent-differences.md); the work itself
remains, and for a large molecule the "Building MO Diagram" progress
dialog appears at open.

A fix has to keep the paths that do show the pane working: the Properties
menu (`OnPropertyMenuClick`, which only shows the frame), a restored
layout, and `ECCE_OPEN_PANEL`.
