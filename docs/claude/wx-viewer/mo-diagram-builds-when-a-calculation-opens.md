---
type: pitfall
title: "A property panel is built when it is on screen, not when wxAUI Show()s it inside a hidden floating frame"
area: wx-viewer
paths: ["src/apps/builder/MoDiagramPanel.C", "src/apps/builder/PropertyPanel.C", "src/apps/builder/BuilderPanels.C", "src/apps/builder/Builder.C"]
issues: []
---
`PropertyPanel::Show(true)` runs `ensureInitialized()`, meant to defer a
panel's work until the user opens it. The MO Diagram prefers floating
(`prefersFloating`), and wxAUI creates the floating frame of a pane at the
first `Update()` even when the pane is hidden: `wxAuiFloatingFrame::
SetPaneWindow` adds the window to the frame's own manager as a shown
centre pane, whose `Update()` calls `window->Show(true)`. Building on that
call computed the whole diagram (point group via autosym, symops, overlap
integrals, Lowdin, the "Building MO Diagram" progress dialog for a large
molecule) for every calculation with orbital energies as the Builder opened
it, with nobody looking.

The fix is general, not MoDiagramPanel-specific:

- `PropertyPanel::Show(true)` initialises only when its top-level window is
  shown. The hidden floating frame is not, so that call does nothing.
- `Builder::initShownPanels()` runs at the end of `updatePanes()` and in
  `Builder::Show(true)`: every shown property pane whose window
  `IsShownOnScreen()` is initialised. This covers the paths that open a
  pane by `pane.Show(true)` plus `updatePanes()` (Properties menu, list
  selection, accordion unfold, restored layout, `ECCE_OPEN_PANEL`), whose
  floating frame only becomes visible inside `Update()`.

A new way of opening a pane must end in `updatePanes()` (or call
`ensureInitialized()` itself), or the panel stays empty. Check with
`ECCE_MODIAGRAM_DUMP` and `strace -f -e trace=execve`: no autosym at open,
one when the MO Diagram is chosen.
