---
type: map
title: "Move to... / Copy to... (#929) are Cut/Copy + Paste with a chosen folder; tree drag auto-scrolls"
area: services
paths: [src/apps/organizer/CalcMgr.C, src/apps/organizer/FolderChooserDialog.C, tests/apps/organizer_moveto_test.py]
issues: [929, 890]
---
`CalcMgr::transferTo(cut, dest)` is the one path: it checks `moveToRefusal`
(the paste/drop rules as text, also what greys OK in `FolderChooserDialog`),
runs `copySelectionToClipboard`, selects `dest` in the tree and calls
`paste()`, so cross-tree copies and refusals are those of the clipboard route.
The user's own clipboard is restored afterwards. `paste()` shows modal
dialogs for its refusals, so `moveToRefusal` must say no first. Hook:
`server moveto copy|cut SOURCE FOLDER`. The dialog builds its tree lazily
from the Organizer's own tree (`findNode` + `loadChildren`). Drag auto-scroll
is `dragScrollTick`, a 80 ms timer started by `enableTreeDropHighlight`.
