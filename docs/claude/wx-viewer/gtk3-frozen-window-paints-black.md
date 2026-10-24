---
type: pitfall
title: "GTK3: a frozen window is painted black when the frame draws before the thaw"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["src/apps/organizer/CalcMgr.C"]
issues: [523, 254, 296]
---
`Freeze()` keeps a wxGTK3 widget from drawing, but GTK3 still draws the
top-level window, and a frozen widget's area then comes out black (tree,
work area, message pane, and the menu bar when the `wxMenuBar` is frozen;
the toolbar and anything not frozen stay painted). In the Organizer,
clicking a GROMACS study blanked the frame for 60-140 ms on Xvfb with
`xcompmgr`, about 0.4 s on GNOME with a data server.
The Organizer's `FrameHold`, `MenuBarHold` and the hold in
`applyQueuedUpdates()` exist for wxMSW, which paints every intermediate
state; GTK paints once per idle and needs none of them, so they freeze
on MSW only. Do not add a `Freeze`/`wxWindowUpdateLocker` around slow work
on GTK. Check: record the Xvfb display (`ffmpeg -f x11grab -framerate 50`)
with `xcompmgr` running and count frames whose black fraction jumps
(plain Xvfb without a compositor was not tried).
