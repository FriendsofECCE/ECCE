---
type: map
title: "wx3.2/GTK3 layout reentrancy"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["docs/HISTORY.md"]
issues: []
---
**wx3.2/GTK3 layout reentrancy**: `wxWindow::DoSetSize` → `wxEVT_SIZE`
→ `Layout()` → reposition children → another `DoSetSize`, sometimes
non-convergent (stack-overflow crash) or asynchronous (crashes well
after `Show()`/`wxEVT_SHOW` return). Not present in GTK2/wx2.8; this is
new surface area from the wx3.2 GTK3 backend. If a dialog crashes or
hangs on construction or first `Show()`, suspect this before anything
else — see `docs/HISTORY.md` for the working detection technique
(backtrace-based reentrancy check in a `wxEventFilter`, not a fixed
timer).
