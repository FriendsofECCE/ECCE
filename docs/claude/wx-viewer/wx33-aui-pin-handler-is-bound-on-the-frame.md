---
type: pitfall
title: "wx 3.3 binds wxAuiManager's pane-button handler on the frame: a handler on the manager never sees the pin button (#344)"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/apps/builder/BuilderPbcTest.C]
issues: [344]
---
The Builder's fold button is AUI's pin button. wx 3.2 handles
`wxEVT_AUI_PANE_BUTTON` in wxAuiManager's static event table, which
`ProcessMgrEvent` reaches after the frame, so a handler bound on the manager
ran and kept the default (float the pane) from running. wx 3.3 (the macOS
build) binds `wxAuiManager::OnPaneButton` on the managed frame in
`SetManagedWindow`; the frame handles the event first, floats the pane, and
the manager's own handlers never run. The fold button floated panes on macOS
only.

The Builder now binds its handler on the frame after `SetManagedWindow`;
dynamic handlers run newest first, so it precedes wx's in 3.3 and the
frame-before-manager order covers 3.2. Scene command `foldclick <pane>`
dispatches the event the way the manager does; `panestate` then reports
`floating` and `folded`.
