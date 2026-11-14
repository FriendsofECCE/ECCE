---
type: pitfall
title: "Menu bars in tabs: the tool frame's GetMenuBar() is null, the Organizer's is the tool's, and wxFrameBase::SetMenuBar compares with the virtual GetMenuBar() (#983)"
area: services
paths: ["src/wxgui/jms/ToolTabs.C", "include/wxgui/ToolTabs.H", "src/apps/builder/Builder.C", "src/apps/builder/Builder.H", "src/apps/organizer/CalcMgr.C", "src/apps/organizer/CalcMgr.H"]
issues: [983]
---
- `ToolTabs::adopt` detaches the tool's menu bar (`SetMenuBar(0)`) and
  swaps it onto the Organizer's frame on tab change.
- Tool side: code doing `GetMenuBar()->FindItem(...)` gets null in a tab.
  `Builder::GetMenuBar` (virtual in wxFrameBase) returns
  `ToolTabs::menuBar(this, wxFrame::GetMenuBar())`, the tab's bar, but only
  while hosted.
- Organizer side: while a tool tab is active, the Organizer frame's
  `GetMenuBar()` is the tool's bar. `CalcMgr::updateSelectionMenus` did
  `GetMenu(FindMenu("Run Mgmt"))` on it: `wxNOT_FOUND`, a garbage menu,
  SIGSEGV in `clearMenu` when the tree took focus. CalcMgr uses
  `ownMenuBar()` (`ToolTabs::mainMenuBar`) everywhere instead.
- Do not solve either by overriding `GetMenuBar()` unconditionally:
  `wxFrameBase::SetMenuBar` returns early when the new bar equals
  `GetMenuBar()`. A Builder override that always returned the tab's bar
  made `release` leave the Open in Window frame with no menu bar; an
  Organizer override would stop `placeMenus` swapping bars. Inside
  ToolTabs.C the frame's real bar is `wxFrame::GetMenuBar()`.
- Check: `towindow` output marks a window frame without a bar
  `(no menus)`; one_process_test fails on it.
