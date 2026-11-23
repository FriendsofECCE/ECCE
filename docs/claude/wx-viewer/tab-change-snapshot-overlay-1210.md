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

Not yet sufficient (first measurement, Xvfb): with it on, the half-drawn
time fell (open Viewer 140 -> 36 samples, open Editor 27 -> 45 but at 3 %
off the old page, which may be the Organizer's own "starting" status), but
closing onto the Editor still showed a mostly flat page for ~280 ms after
the picture went. Ending on "no wxEVT_PAINT" misses native GTK widgets, which
draw without a wx paint event, so the picture can go before they are drawn.
