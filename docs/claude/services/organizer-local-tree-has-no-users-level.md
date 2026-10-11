---
type: map
title: "In local data mode the Organizer's root is the user's home, labelled Local data"
area: services
section: "Pitfalls"
paths: ["src/wxgui/wxtools/WxResourceTreeCtrl.C", "src/apps/organizer/CalcMgr.C", "src/dsm/edsiimpl/EDSIServerCentral.C", "tests/apps/organizer_local_tree_test.py"]
issues: [216]
---
The local data folder holds `users/local` and nothing else the user can
see, so the tree used to read "Local data > users > local > projects".
`WxResourceTreeCtrl::addRoot` now roots the node named "Local data" at
`EDSIServerCentral::getUserHome()` (`<dir>/users/local`), as "Set Home as
Root" does, so the projects are its direct children. Display only: the
layout, every resource URL and the data server's own listing are
unchanged, and a server root is never touched (`isLocalDataFolder` needs
`LocalData::dir()` set). With "Set Home as Root" on, the root keeps its
label "Local data - local".

What this relies on: `findNode` matches by URL (a URL above the home, such
as the data folder itself in an old history entry, is simply not found
and `autoOpen` falls back to the home), `openHome` finds the home through
`getUserHome(root url)`, which is the root's own URL, and `selectParent`
stops at the first level under the hidden root. Bookmarks and the
Organizer history hold plain URLs; the path in tool status bars comes from
`mapURLtoName` (URL based, "(HOME)/project"), so none of them change.

`ECCE_TEST_ORGANIZER` has `tree`, `up` and `home` commands for this
(tests/apps/organizer_local_tree_test.py).
