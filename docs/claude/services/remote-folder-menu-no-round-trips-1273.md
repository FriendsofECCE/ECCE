---
type: pitfall
title: "A right-click menu on an SSH folder must not look anything up on the server (#1273)"
area: services
paths: ["src/apps/organizer/CalcMgr.C", "src/dsm/edsiimpl/SftpEDSI.C", "src/apps/organizer/ProjectContextPanel.C", "tests/apps/ssh_root_menu_test.py", "tests/apps/fakessh/ssh"]
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
- Selecting an SSH folder made three scripts on the GUI thread (GTK selects
  a node before its right-click menu): the tree's listing in
  `Resource::getChildren`'s own Batch, the same listing again in
  `ProjectContextPanel::initialize`'s, and the auto-refresh `stat`
  (`SftpEDSI::folderStamp`). `CalcMgr::onSelectionChange` now holds one
  Batch over all three and `folderStamp` answers from that Batch's listing,
  so a selection is one script. The test checks `select` + `desk` costs one
  script and under 1.5 x the fake delay.
