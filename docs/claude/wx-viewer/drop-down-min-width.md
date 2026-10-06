---
type: rule
title: "Every drop-down is at least as wide as its widest entry"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["src/wxgui/ewxClasses/ewxStyledWindow.C", "src/wxgui/ewxClasses/ewxChoice.C", "src/wxgui/ewxClasses/ewxComboBox.C", "include/wxgui/ewxStyledWindow.H", "include/wxgui/ewxChoice.H", "include/wxgui/ewxComboBox.H"]
issues: [210]
---
**Every drop-down is at least as wide as its widest entry**, in the
current theme font (#210). The rule lives in one place,
`ewxStyledWindow::fitDropDown()`, and `ewxChoice`/`ewxComboBox` call it
on creation and from their `DoInsertItems()` override, so entries added
later (`Append`, `Insert`, `Set`) widen the control too. A larger width
given at construction is kept; a smaller one, which the DialogBlocks
code is full of (`wxSize(50, -1)`), no longer clips. Clearing never
shrinks the control.

Do not size a drop-down by hand in a window. A plain `wxChoice`/
`wxComboBox` is outside the rule: use the ewx class, or call
`fitDropDown()` after filling it. A drop-down that starts empty and is
filled only when data arrives has nothing to measure; give it a
minimum width from its longest expected value (see `NWDirdy::
createDynamicGUI()`).
