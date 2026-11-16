---
type: pitfall
title: "wxAuiManager::SetManagedWindow pushes the manager on the window's handler stack: PopEventHandler(true) in a destructor then frees a member (#983)"
area: services
paths: ["src/apps/builder/Builder.C", "src/wxgui/jms/ToolTabs.C", "src/wxgui/ewxClasses/ewxFrame.C"]
issues: [983]
---
- `SetManagedWindow(w)` does `w->PushEventHandler(this)`; `UnInit()` does
  `RemoveEventHandler(this)`. The handler order on the frame therefore
  depends on when the manager was last pointed at it.
- `ToolTabs::release(this, false)` in `~Builder` points the manager back at
  the frame, after the Builder's own `ViewerEvtHandler` was pushed. The
  next line, `PopEventHandler(true)`, popped and deleted the manager (a
  member of Builder): `free(): invalid pointer`, SIGABRT at Quit.
- Remove a known handler by pointer (`RemoveEventHandler(h); delete h;`),
  never `PopEventHandler(true)`, on a frame whose AUI manager can move.
  `~ewxFrame` still pops: it is safe only because Builder's `p_mgr`
  member is destroyed (UnInit) before the base destructor runs.
