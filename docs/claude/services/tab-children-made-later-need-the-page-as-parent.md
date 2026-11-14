---
type: pitfall
title: "In a tab, GetSizer/FindWindow/Layout of the frame and controls made with `this` as parent go to the wrong place"
area: services
paths: ["src/apps/calced/CalcEd.C", "src/wxgui/jms/ToolTabs.C"]
issues: [983]
---
Once `ToolTabs::adopt` moved a tool's windows and sizer into the page, code
that treats the hidden frame as the container fails silently:

- `GetSizer()` is null: use `ToolTabs::sizer` (`CalcEd::mainSizer()`).
- `FindWindow(id)` does not find the controls: `ToolTabs::client(this)->FindWindow`.
- `Layout()` lays out nothing: `ToolTabs::layout`.
- `new wxWindow(this, ...)` made later (the editor rebuilds its code
  buttons and summary labels on every context change) is a child of the
  hidden frame, so it is never drawn, though the sizer holds it and its
  size is stale. Parent such controls with `ToolTabs::client(this)` when
  hosted (`CalcEd::clientParent()`). Found as an empty Code row and
  missing Settings rows in the tab, fine in a window.
- `SetSizeHints(this)`/`fitToScreen()` act on the frame; a page needs a
  scroller instead (`ToolPage` with a `wxScrolledWindow` body).
Check with `tabinfo Launch...,Save,Verify` (reachable, no `STALE`).
