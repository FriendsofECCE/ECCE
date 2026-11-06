---
type: pitfall
title: "~/.ECCE/ServerList belongs to the legacy server bookmarks; the added-server list is ~/.ECCE/AddedServers"
area: services
paths: [src/util/genutil/ServerList.C, src/apps/organizer/CalcMgr.C, src/wxgui/ewxClasses/ewxGenericFileDialog.C]
issues: [767]
---
The Organizer's server bookmark editor and the generic file dialog read
`~/.ECCE/ServerList` as an XML bookmark list (`BookmarkList("ServerList")`).
A plain-text file of the same name makes BookmarkList abort with "XML Parse
FATAL" at the next Organizer start. The list of servers added in
Preferences > Data (ServerList.H, `ecce-localdata servers`) is therefore kept
in `~/.ECCE/AddedServers`. Before naming any new file in the user preference
directory, grep for the name in `BookmarkList(`, `BookmarkEditor(` and the
other readers of `Ecce::realUserPrefPath()`.
