---
type: map
title: "Builder panel layouts (View > Panel layout): every pane stays an AUI pane; the one-column modes hide the inactive tab's panes"
area: wx-viewer
paths: [src/apps/builder/BuilderPanels.C, src/apps/builder/Builder.C, src/apps/builder/PropertyIndexPanel.C, data/client/config/PropertyPanelDescriptor.xml]
issues: []
---
Four layouts, saved as `/PanelMode` in `wxbuilder.ini`: `classic` (tools right,
property panels left, as before), `stacked`, `accordion`, `detail` (the default).
The three one-column layouts dock a "Panel Tabs" pane, then either the tool
panes (Structure tab) or the property panes (Properties tab), in one Right
dock on layer 1 (full height); the Log is a Bottom pane under the viewer,
folded to its caption line and unfolded by `ewxLogTextCtrl::setMessageHandler`.

- Panes of the inactive tab are hidden by `Builder::syncColumn()` and remembered
  in `p_tabHidden`, so "closed by the user" and "hidden by a tab / F9" stay
  distinct. Menu ticks use `paneWanted()`, never `IsShown()`. `paneInfoForSave`
  saves a tab-hidden pane as shown, or the layout would reopen empty.
- Every `p_mgr.Update()` in Builder.C goes through `updatePanes()`: it applies
  the layout rules first and re-hides folded windows afterwards (Update() re-shows
  them), under the `p_panelBuildDepth` guard so re-docking cannot activate a
  viz overlay (#111). A bare `p_mgr.Update()` brings back a folded pane's window.
- Saved tool layouts are per family: `/PaneLayout/` (classic, the old key) and
  `/PaneLayoutColumn/`. Dock direction, layer and position are forced again on
  every load in the column modes (`applyGeometry`), only visibility and sizes persist.
- Migration (`initPanelMode`, no `/PanelMode` yet): a saved `/PaneLayout/Default`
  and `ReadOnly` whose every tool pane has the old default's shown/hidden flag
  (and the shown ones docked right) is deleted and the user gets list + detail;
  anything else maps to Classic. Positions, sizes and toolbars are ignored.
  The version-upgrade reset in `restoreSettings` still deletes layouts afterwards.
- A panel that `prefersFloating()` (MO Diagram) stays floating in every layout
  (group none); it is shown and raised from the list or the menu.
- The list's groups come from `<group>` in `PropertyPanelDescriptor.xml`
  (missing = "Other"); an installed descriptor without it lists everything
  under Other.
- Hooks: `ECCE_PANEL_MODE`, `ECCE_PANEL_TEST=<file>`, `ECCE_PANEL_METRICS=<file>`,
  `ECCE_PANEL_FULLSCREEN`; driver `tests/panels/capture.py`.
