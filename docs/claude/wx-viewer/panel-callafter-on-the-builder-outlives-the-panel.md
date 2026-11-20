---
type: pitfall
title: "A panel's CallAfter queued on its Builder outlives the panel (one process)"
area: wx-viewer
paths: ["src/apps/builder/MoPanel.C", "src/apps/builder/PropertyPanel.C", "tests/apps/one_process_test.py"]
issues: [1141, 1136, 983]
---
`builder->CallAfter([this] { ... })` from a PropertyPanel queues the call on
the Builder, not on the panel. When the panel is destroyed first (a tab
closed, its panels freed), the queued lambda runs on a dead `this` and the
Organizer segfaults in idle processing (`MoPanel::reflow` in
`one_process_test.py --1088`, after ba9a2f086 made `PropertyPanel::builder()`
find the Builder inside a tab, where `wxGetTopLevelParent` had returned the
Organizer frame and so never queued anything). Queue a panel's deferred work
on the panel itself (`CallAfter(...)`, which wx drops with the panel) and
look the Builder up inside the call.
