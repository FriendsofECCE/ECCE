---
type: pitfall
title: "wx 3.3 binds wxAuiManager's pane-button handler on the frame, wx 3.2 pushes the manager: the fold handler must be on both (#344)"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/apps/builder/BuilderPbcTest.C, tests/apps/builder_panes_test.py]
issues: [344]
---
The Builder's fold button is AUI's pin button, and wx's own pin handler
floats the pane. `ProcessMgrEvent` sends `wxEVT_AUI_PANE_BUTTON` to
`frame->GetEventHandler()` first, then to the manager.

- wx 3.2 (Linux, Windows) pushes the manager onto the frame
  (`PushEventHandler`), so the frame's handler chain starts at the manager;
  a handler bound on the manager runs before the manager's static table.
  A handler bound on the frame only runs after wx has floated the pane.
- wx 3.3 (macOS build) pushes nothing and binds `OnPaneButton` on the
  frame in `SetManagedWindow`; the frame floats the pane and the manager's
  own handlers never run.

The Builder binds `OnPaneButton` on both, the frame one after
`SetManagedWindow` (newer dynamic handlers run first). The pin event is
never skipped, so it is handled once. Binding only on the frame floated the
pane on Linux.

Scene command `foldclick <pane>` dispatches the event the way the manager
does; `builder_panes_test.py` folds Build with it and checks `floating`.
