---
type: pitfall
title: "wxMSW: a control made as a child of a tool panel after scrollToolContent() sits under the scroll window and never gets a click (Build pane element buttons)"
area: wx-viewer
paths: ["src/apps/builder/Builder.C", "src/apps/builder/MiniPerTab.C"]
issues: [254]
---
`Builder::scrollToolContent()` moves a tool panel's controls into an
`ecceToolScroll` wxScrolledWindow that fills the panel. Controls the panel
creates later with `this` as parent (`MiniPerTab::setElements()` runs again from
`restoreSettings()`) are siblings of the scroll window and lie beneath it. On
GTK that still worked; on MSW `WindowFromPoint` at the centre of each of the
twelve element toggle buttons, Delete and More... returned the scroll window,
so they were visible and enabled but dead (Add H / Del H, made before the move
and so inside it, worked). The Build pane could never change the element and
a selected atom answered "The new atom and shape are identical...".

- A tool panel that builds controls after `addToolPanel()` must parent them to
  its `ecceToolScroll` child (see `MiniPerTab::setElements`).
- Test hooks bypass hit testing, so two rounds could not reproduce it. Check
  with `WindowFromPoint` at a control's centre (EnumChildWindows lists the
  siblings top first); a disabled control is skipped by `WindowFromPoint`, and
  one scrolled out of view is clipped, both are expected misses.
