---
type: pitfall
title: "The PropertyPanel registry is process-wide: a Builder must ask for its own panels (#1139)"
area: wx-viewer
paths: ["src/apps/builder/PropertyPanel.C", "src/apps/builder/VizPropertyPanel.C", "src/apps/builder/MoDiagramPanel.C", "src/apps/builder/MoPanel.C", "src/apps/builder/PointNote.C", "src/wxgui/jms/ToolTabs.C", "src/apps/builder/PropertyPanel.H", "src/apps/builder/Builder.C", "src/apps/builder/BuilderPanels.C", "tests/apps/one_process_test.py"]
issues: [1139, 1131, 1135, 983, 1141]
---
`PropertyPanel::getPanels(url)` and `panelExists(url, name)` return every
panel for that URL in the process. With one program (#983) a Builder and a
Viewer on the same calculation are two `Builder`s in one process, and
each used to add the other's panels to its own `wxAuiManager`
(`updatePropertyMenus()`, `loadPaneLayout()`) and skip creating its own.
Closing one tab destroyed its panels while the other manager still listed
them; the survivor's next `savePaneLayout()` crashed on the freed window.

Each panel records its owner (the window passed to `Create()`, kept when
tabs reparent it). Code that touches a Builder's panes passes `this`:
`getPanels(url, "", this)`, `panelExists(url, name, this)`,
`getPanelsForPropertyName(url, key, this)`. Calls without an owner still
see every panel; use them only for something meant to cross tools.
Panels looking for a sibling (MO diagram -> MOs, Density Map ->
profile, IRC -> MOs) pass `getOwner()`; `VizPropertyPanel::getPanels()`
takes the same owner argument.

A panel finds its Builder with `PropertyPanel::builder()` (the owner), any
other window with `Builder::containing(w)`. Never `GetParent()` or
`wxGetTopLevelParent()`: hosted as a tab, a Builder's windows are children
of the tab page under the Organizer's frame, so both miss it and the code
silently does nothing (#1141: empty Diffraction pane).
Test: `tests/apps/one_process_test.py --1131`.
