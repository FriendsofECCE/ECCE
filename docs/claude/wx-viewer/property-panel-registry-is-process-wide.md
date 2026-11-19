---
type: pitfall
title: "The PropertyPanel registry is process-wide: a Builder must ask for its own panels (#1139)"
area: wx-viewer
paths: ["src/apps/builder/PropertyPanel.C", "src/apps/builder/PropertyPanel.H", "src/apps/builder/Builder.C", "src/apps/builder/BuilderPanels.C", "tests/apps/one_process_test.py"]
issues: [1139, 1131, 1135, 983]
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
`VizPropertyPanel::getPanels()` has no owner filter yet (quit and
closeContext unfocus every viewer's panels for the URL).
Test: `tests/apps/one_process_test.py --1131`.
