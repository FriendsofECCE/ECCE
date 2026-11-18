---
type: pitfall
title: "macOS: wx's automatic Window menu empties any menu titled \"Window\" natively; deleting a wx item from it raises an NSMenu exception (#1119)"
area: wx-viewer
paths: ["src/wxgui/jms/ToolTabs.C"]
issues: [1119, 1069, 983]
---
- wxOSX (3.2 and 3.3) has one application menu bar. Each time a bar is
  installed (`wxMenuBar::MacInstallMenuBar` -> `MakeRoot`, i.e. on
  `SetMenuBar` of the front frame and on every frame activation),
  `MacSetupWindowMenu` (src/osx/cocoa/menu.mm) finds the menu titled
  "Window", calls `[windowMenu removeAllItems]`, puts in Minimize/Zoom/Bring
  All to Front and hands it to `[NSApp setWindowsMenu:]`.
- The wxMenu still lists its old items. `wxMenu::Delete`/`Destroy` of one
  then calls `[NSMenu removeItem:]` on an item no longer in the menu:
  `*** Assertion failure in -[wxNSMenu removeItem:], NSMenu.m:875`, an
  uncaught NSException that ends the main loop.
- ToolTabs's Window menu (tab list, Move to Tabs) is exactly such a menu,
  rebuilt with `Delete` on every bar swap and menu open, so on macOS every
  tool opened as a tab ended ECCE. Fix: `wxMenuBar::SetAutoWindowMenu(false)`
  in `ToolTabs::setHost`, before any bar is installed. The price: no
  Minimize/Zoom items or Cocoa window list in that process's Window menu.
- Any other code that keeps a wx menu titled "Window" on macOS and edits it
  has the same fault while the automatic Window menu is on.
- After the exception wx cleans up with the host frame already destroyed;
  `ToolTabs::release` from `~CalcMgr` must not touch `s_book`/`s_hostFrame`
  then (the segfault in the #1119 crash report).
- Evidence: `ECCE_DEBUG_MENUS=1` prints every bar set/attach/detach/fill on
  stderr.
