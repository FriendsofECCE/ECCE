---
type: rule
title: "A property panel's overlay follows the panel: opened = drawn, closed/replaced/folded = removed; \"Show in viewer\" turns it off"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/apps/builder/BuilderPanels.C, src/apps/builder/VizPropertyPanel.C, src/apps/builder/BuilderScript.C, src/apps/builder/PropertyIndexPanel.C, tests/apps/builder_overlay_test.py]
issues: [111]
---
The legacy rule (`VizPropertyPanel::doFocus`): one panel per calculation
holds viz focus; taking it calls `loseFocus()` on the panel that had it,
which restores the plain display. The ewxAUI caption buttons that drove it
are gone (see the AUI-port entry); the triggers now are:

- **shown by the user** (Properties menu, the Properties list in list +
  detail, unfolding an accordion caption, `ECCE_OPEN_PANEL`):
  `Builder::focusShownPanel()`. Keyboard focus does not move into a newly
  shown pane on GTK or MSW, so `OnChildFocus` alone left nothing drawn until
  a click in the pane. The list path (`selectDetail`) was missed by the first
  fix, which was checked only with `ECCE_OPEN_PANEL`.
- **hidden** (menu untick, pane close, replaced in list + detail by
  `setDetail`, folded, its tab hidden by `syncColumn`): `unfocusPanel()`.
  `setDetail` hides the old pane itself, so `syncColumn`'s own unfocus never
  sees it shown; it must unfocus there.
- **the panel's "Show in viewer" box** (added under every panel that
  `drawsInViewer()`, on first show): the explicit off switch.
  `OnChildFocus` ignores focus landing on the box, or the click that ticks
  it would first take focus and then untick it.
- Opening a calculation never activates an overlay (#111).

Check with the real event paths, not a hook that skips them:
`tests/apps/builder_overlay_test.py` drives the Builder script commands
`list` (the tree's own selection event), `property` (a checked menu event)
and `viewer` (a checkbox event), and counts the dipole's yellow and
Mulliken's blue pixels inside the viewer.

The column's first tab follows the calculation (Properties with results,
Structure without); a tab clicked applies to that calculation only. It used
to be saved as `/ColumnTab`, so one click on Structure opened every
finished calculation there afterwards.
