---
type: pitfall
title: "A property panel's SetSizeHints() lifts its docked pane's floor to the content height"
area: wx-viewer
paths: ["src/apps/builder/PropertyPanel.C", "src/apps/builder/PropertyPanel.H", "src/apps/builder/Builder.C", "src/apps/builder/MoPanel.C", "src/apps/builder/KeyValuePropertyPanel.C", "src/apps/builder/TablePanel.C"]
issues: []
---
`addPropertyPanel()` gives a docked property pane a small `MinSize` floor
(80 px, or `minimumHeight()`) and shares the dock by `dock_proportion`.
wxAUI also floors each pane at its **window's** minimum, because the
window sits in the pane's own sizer. Many panels call
`GetSizer()->SetSizeHints(this)` in `Create`, `initialize()` or
`refresh()` (`MoPanel`, `KeyValuePropertyPanel`, `TablePanel`, ...),
which sets the window's minimum to the full content height.

Calculation Summary (359 px) and Energies, shown by default in the same
left dock, therefore took a 1366x768 screen's dock. The MOs pane got
only its caption in the classic layout (400x21) and 0 px in the stacked
one, whatever its proportion. `tests/apps/panel_layouts_test.py`
(`apps_panel_layouts`) catches this.

`PropertyPanel::setPaneMinHeight()` (called by `addPropertyPanel()`) and
the `SetMinSize` override keep the window's minimum height at the pane's
floor, so every path that sets size hints is covered. A new panel can
call `SetSizeHints` freely. A panel that really needs height says so
through `minimumHeight()`, not through its window's minimum.

Diagnose with `ECCE_DEBUG_PANEL_SIZE=1` (`[PANELSIZE]`/`[PANESIZE]`
lines) and `ECCE_PANEL_METRICS=<file>` (each shown pane's rectangle).
