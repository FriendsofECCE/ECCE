#!/usr/bin/env python3
"""Close a window by title the way a window manager's close button does:
a WM_DELETE_WINDOW client message. Not keyboard or pointer input.

    closewin.py <display> <title>        exit 0 if a window was asked to close
"""
import subprocess
import sys

from Xlib import X, display, protocol

name, title = sys.argv[1], sys.argv[2]
tree = subprocess.run(["xwininfo", "-display", name, "-root", "-tree"],
                      stdout=subprocess.PIPE, text=True).stdout
wid = None
for line in tree.splitlines():
    parts = line.strip().split(None, 1)
    if len(parts) == 2 and parts[0].startswith("0x") and \
            parts[1].startswith('"%s"' % title):
        wid = int(parts[0], 16)
        break
if wid is None:
    sys.exit(1)
conn = display.Display(name)
win = conn.create_resource_object("window", wid)
event = protocol.event.ClientMessage(
    window=win, client_type=conn.intern_atom("WM_PROTOCOLS"),
    data=(32, [conn.intern_atom("WM_DELETE_WINDOW"), X.CurrentTime, 0, 0, 0]))
win.send_event(event, event_mask=X.NoEventMask)
conn.flush()
conn.close()
