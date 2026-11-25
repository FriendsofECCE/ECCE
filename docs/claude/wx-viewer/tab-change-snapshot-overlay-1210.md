---
type: pitfall
title: "A tab change is covered by a picture of the old page until the new one has drawn; freezing the host frame instead shows it black on GNOME (#1210)"
area: wx-viewer
paths: ["src/wxgui/jms/ToolTabs.C", "tests/apps/screenframes.py", "tests/apps/one_process_test.py"]
issues: [1210, 983]
---
A page of the tool notebook that was hidden is mapped and drawn by GTK over
several frame-clock cycles, so opening, closing or switching a tab showed a
half-drawn page (grey window, empty side panel, a spinner) for 2-10 frames.

`wxWindow::Freeze` on the host frame (or `wxWindowUpdateLocker` around
`adopt`/`release`) is not a fix: under GNOME the frozen toplevel is shown
BLACK for its duration (~200 ms), and adopt runs without a loop pass anyway.

`beginSnapshot()` in `ToolTabs.C`, called from adopt, release (selected tab
only), select and `wxEVT_AUINOTEBOOK_PAGE_CHANGING`: reads the selected
page's area back from the X server (`gdk_pixbuf_get_from_window` on the
toplevel, so the GL canvas is in it), puts a child window with that picture
over it, then the change happens underneath.  The child must be made native
(`gdk_window_ensure_native`) and raised on every tick, since the Viewer's GL
canvas is a native X window and covers any client-side window; its input
shape is empty, so clicks reach the page, and it never takes focus.  It is
allocated by hand: a new child of a frame whose size does not change is never
allocated by GTK (gtk-reparented-children-never-allocated.md).  It goes when
no host window painted for two 16 ms ticks (at least one frame), when a modal
dialog opens (wxModalDialogHook), or after 600 ms; then the host is
refreshed.  A second change while it is up keeps the first picture.

`gtk_widget_draw` of the notebook was rejected: the GL canvas comes out
empty, and it runs every wx paint handler outside the frame cycle.
X11 only (`GDK_IS_X11_DISPLAY`); on Wayland, Windows and macOS there is no
picture and tabs change as before.  `snapshot off|on` / `ECCE_TAB_SNAPSHOT=0`
switch it; `one_process_test.py --1210-frames` measures what reaches the
screen with it off and on.

Measuring it: compare colour only.  On a depth-24 visual (Xvfb, most X
desktops) XGetImage gives 32-bit pixels whose top byte is not shown and
differs between the X server's copy of a GTK drawing (0xff) and the same
colours drawn from the picture (0x00).  `screenframes.Sampler` clears that
byte; before it did, the picture itself read as 3 % (Organizer) or 13 %
(Editor) "off" the page it showed, and the old page appeared "flat" after
a close, though the colours on the screen were identical.  With colour only,
every state while and after the picture is up is within 2 % of the old page
or the finished one.  The reference "before" is the state on the screen when
the command was written, not the first sample: a status line can change in
the half second before it.

One state is left, and only where the change ends on a Viewer (open
Viewer, open Viewer over Editor): a single sample (about 12 ms, 53 % off the
old page, 32 % off the new one) just after the picture is removed, in 1-2
of 6 cases a run.  Nothing under the native picture window is kept by the
X server, the GL canvas included, so both are drawn again after removal and
the canvas lands a frame after the rest.  `Refresh()` + `Update()` of the
host in `endSnapshot` (the canvas paints synchronously in `OnPaint`) did not
change it.  Not yet tried: a separate popup toplevel instead of a child
window, so that a compositing desktop keeps the page drawn beneath it.

Measured on Xvfb (no compositor) only; a compositing desktop draws the window
from its own copy and has not been measured.
