---
type: pitfall
title: "XWayland: a GTK 3 window stops repainting after its first frame or two when Mutter sends no _NET_WM_FRAME_DRAWN; ewxModalGuard turns frame sync off for every top-level window"
area: wx-viewer
paths: ["src/wxgui/ewxClasses/ewxModalGuard.C", "include/wxgui/ewxModalGuard.H", "CMakeLists.txt", "src/apps/builder/BuilderScript.C", "tests/apps/xwayland_paint_test.py"]
issues: [311, 332, 120]
---
GTK 3.24 on X11 (`gdk/x11/gdkwindow-x11.c`, `gdkdisplay-x11.c`): a
top-level window has an extended `_NET_WM_SYNC_REQUEST` counter when the
window manager supports `_NET_WM_FRAME_DRAWN` (Mutter does, also under
XWayland). After every frame, `gdk_x11_window_end_frame` sets
`frame_pending` and freezes the window's frame clock. Only a
`_NET_WM_FRAME_DRAWN` client message from the compositor (or an
UnmapNotify) thaws it; there is no timeout. While it is frozen GTK handles
input (keys reach a GtkEntry, Enter still applies the text) but draws
nothing.

What `ECCE_DEBUG_MODAL` showed on beryllium (Debian 13, GNOME 48,
XWayland): a failing dialog stays at `draws=1..2`, `frame=0..1` while its
`input` lines keep coming; a working one goes past `frame=12` within the
first half second. It alternated between dialogs, not dialog types (New
Calculation worked, the next New Calculation did not; Color Chooser and
Verify failed and then worked on a retry). A late `_NET_WM_FRAME_DRAWN`
unfreezes it, which is why the second Verify started scrolling after a few
wheel turns. The XMODIFIERS/input-method and popup-menu correlations in
`xwayland-modal-dialog-takes-no-input.md` were this intermittent stall.

Reproduced headless: `gnome-shell --headless --wayland --virtual-monitor`
(own D-Bus session daemon, `XDG_RUNTIME_DIR` short enough for the 108-byte
socket path, `XAUTHORITY` = `$XDG_RUNTIME_DIR/.mutter-Xwaylandauth.*`,
Xwayland display from the `/tmp/.X<n>-lock` holding gnome-shell's pid). A
plain GTK 3 program opening modal windows that change a label every 40 ms:
12 of 12 stuck at frame 0..1 with no `_NET_WM_FRAME_DRAWN`; with
`gdk_x11_window_set_frame_sync_enabled(win, FALSE)` 0 of 12 (about 32
frames each in one second).

The same stall hits every top-level window, not only dialogs. In the
Builder it looks like broken clicks: under the headless shell with frame
sync on, a click on an H in Atom mode turned it into O and a click in free
space added an atom (the fragment changed), but the Builder window's frame
counter stayed at 0, so the 3-D view (a wxGLCanvas, drawn from GTK's
`draw`) never showed it. The same clicks through XTest on Xvfb work.
`Builder::guardSashDrag` is not involved: it acts only on mouse events of
the frame itself, and the canvas never holds the capture.

`gdk_x11_window_set_frame_sync_enabled(window, FALSE)` (public since 3.8,
meant for XEMBED windows; it only ever disables) makes `end_frame` skip
the freeze; GTK still steps the sync counter, so Mutter still knows when a
frame is complete. `ewxModalGuard::install()` (from `ewxApp::OnInit`)
does it from an emission hook on `GtkWidget::map` for every GtkWindow, so
for frames, dialogs, native GTK dialogs and menus alike: the hook runs
before the class handler, after realize, before the first frame (a new
top-level is frozen until MapNotify). Only when the X server has the
XWAYLAND extension; `ECCE_FRAME_SYNC=1` keeps GTK's default, `=0` turns
it off on any X server. `modal.log` lines carry `frameSync=on|off` and
`frameDrawn=<n>` (the compositor's replies to that dialog).

Not covered: the wxPython dialogs (`scripts/codereg/*.py`) and other
non-ewxApp programs. Xvfb has no window manager, so GTK never freezes
there. `tests/apps/xwayland_paint_test.py` runs the Organizer's text prompt
and the Builder on the headless shell and checks that their frame counters
advance (Builder script `info` prints `frame=`; `atompos N` / `viewpos fx
fy` print screen positions for a test that clicks through the X server);
it fails with `ECCE_FRAME_SYNC=1`.
