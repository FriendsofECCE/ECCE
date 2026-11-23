---
type: pitfall
title: "A right-click menu on an SSH folder must not look anything up on the server (#1273)"
area: services
paths: ["src/apps/organizer/CalcMgr.C", "tests/apps/ssh_root_menu_test.py", "tests/apps/fakessh/ssh"]
issues: [1273]
---
- `CalcMgr::getContextMenu` runs on the GUI thread outside any
  `SftpEDSI::Batch`, so every `EDSIFactory::getResource` of a URL not in the
  pool is one ssh round trip. The site-screening check
  (`folder.getChild("host")`) cost one per right-click on every folder; on a
  slow cluster link the menu took seconds and GNOME offered to kill ECCE.
  Off `file://` it now asks the tree (`findNode(url, false, false)`), i.e.
  what the listing already showed.
- Rule: a new menu item that depends on the server decides from the tree,
  or when chosen; never by a lookup while the menu is built.
- Test: `tests/apps/ssh_root_menu_test.py` puts `tests/apps/fakessh/ssh` on
  PATH (runs the command locally after a delay; a non-zero port in the
  `sftp://` address forces the ssh transport even for localhost) and times
  the menu with the `menulabels` hook (`select=` and `menu=` ms).
- Not fixed: selecting an SSH folder (its desk) still makes three scripts on
  the GUI thread: a listing outside a Batch, the same listing again inside
  `ProjectContextPanel::initialize`'s Batch, and an mtime `stat`.
