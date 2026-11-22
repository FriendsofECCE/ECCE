---
type: pitfall
title: "Tab tooltips and the Window menu take the calculation from the tool's poll context; Ctrl+Tab works through the key hook, not the menu accelerator (#1256)"
area: services
paths: ["src/wxgui/jms/ToolTabs.C", "src/wxgui/jms/InProcessTools.C", "src/wxgui/jms/WxJMSMessageDispatch.C", "tests/apps/one_process_test.py"]
issues: [1256, 983]
---
- A tab's path is `InProcessTools::context(frame)`: the dispatcher's poll
  context (what every tool sets when it changes calculation), else the URL
  it was opened for, mapped with `EDSIServerCentral::mapURLtoName` as the
  status bar does. `setPollContext` calls `ToolTabs::contextChanged`, which
  refreshes tooltips and the Window menu after the event; a tool that never
  sets a poll context gets no path.
- One tab per calculation and tool is `InProcessTools::open`'s match on app
  plus context (not `forcenew`); a reused window's tab is selected there.
- GTK does not accept Tab as a menu accelerator, so "Next Tab\tCtrl+Tab" is
  only a label on Linux; the host's `wxEVT_CHAR_HOOK` does the switching. If
  a platform delivers both, `cycleTab` drops the second step in the same
  event-loop pass. macOS: Cmd+Tab is the system's app switcher, so the key
  hook uses the raw Control key for Tab and the menu shows Cmd+Shift+] / [.
- Hooks: `tabtips`, `nexttab`, `prevtab`, `tabkey next|prev|pgdn|pgup`,
  `windowitems`; section `runTabIdentity` (`--1256-only`).
