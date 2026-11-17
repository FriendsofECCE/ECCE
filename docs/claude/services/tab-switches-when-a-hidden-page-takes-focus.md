---
type: pitfall
title: "Tabs: wxAuiNotebook selects the page of any window that takes the focus; the Organizer's tree refocus after a tool's Save brought its tab to the front (#1088)"
area: services
paths: ["src/wxgui/jms/ToolTabs.C", "src/apps/organizer/CalcMgr.C", "tests/apps/one_process_test.py"]
issues: [1088, 983]
---
- `wxAuiNotebook::OnChildFocusNotebook` calls `SetSelection` for the page
  holding the newly focused window. Code that focuses a window in a hidden
  page therefore switches tabs; a separate Organizer window never noticed.
- `CalcMgr::stopDisabler` refocuses the tree when `p_focusOnTreeBeforeDisabler`
  is set, and `p_focusOnTree` is effectively always true (`focusLeaveTree`
  is empty). Updates the Organizer runs after a tool's Save, Launch or close
  messages went through it, so Ctrl+S in the Builder, Save or Launch in the
  editor selected the Organizer tab, and the first click on a tab's close
  button only switched tabs.
- `ToolTabs::setMainWindow` binds `wxEVT_CHILD_FOCUS` on the notebook (bound
  handlers run before its event table) and drops focus events for a page
  that is not the selected one; `stopDisabler` skips the refocus while
  `ToolTabs::activeTool()` is set. Select a tab with `ToolTabs::select` or
  `SetSelection`, never by focusing something in it.
- Test hooks `tabsel`, `tabsave`, `tabclose N`, `tabfocus N`; section
  `runTabs1088`.
