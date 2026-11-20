---
type: pitfall
title: "A menu shown with ewxPopupMenu: items Bind()-ed on the menu did nothing, and a connected server root got no server menu"
area: services
paths: [src/wxgui/ewxClasses/ewxWindowUtils.C, src/apps/organizer/CalcMgr.C, tests/launch/ssh_folder_launch_test.py]
issues: [1221, 795, 311]
---
**`ewxPopupMenu` (#311) takes the id from `GetPopupMenuSelectionFromUser`
and sends it afterwards.** It sent it to the window only, so handlers bound
on the `wxMenu` itself (`menu.Bind(wxEVT_MENU, ...)`, as all of
`CalcMgr::serverRootMenu` is) never ran on a real click.  It now tries the
owning menu and its parents (`ProcessEventLocally`) first, then the window.
The `server rootmenu/rootchoose/disconnect` hooks build the menu and call
`menu.ProcessEvent` themselves, so they passed while the real click did
nothing: a menu fix needs a real right-click test (xdotool,
`server rootpos`, `ECCE_TEST_POPUP_LOG`).

**A root is a server root because it carries `serverRef()`** (set only by
`addServerRoot`/`addSshRoot`), not because it has no resource: a connected
SSH or ECCE-server root is also a folder with a resource.
`CalcMgr::OnContextMenu` tested `!getResource()` and so offered no
Disconnect on any connected root.  Now: server items, a separator, then
the folder's items.
