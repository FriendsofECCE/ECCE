---
type: pitfall
title: "Headless GNOME Shell can deadlock in PulseAudio's X11 lookup"
area: wx-viewer
paths: ["tests/apps/xwayland_paint_test.py"]
issues: [1081]
---
An empty modal trace and a Builder log containing only
`ecce-dataserver-start: data server already listening` can mean neither app
got through GTK initialization. The server message is expected: the fixture
starts the isolated server, then each app wrapper ensures it is running.

On NerdEgg, the private headless GNOME Shell blocked in
`pa_client_conf_from_x11` -> `xcb_connect_to_display_with_auth_info`, while
Xwayland blocked in `wl_display_roundtrip_queue` waiting for the compositor.
The apps then blocked in `XOpenDisplay`. PulseAudio performs this lookup
when `SSH_CONNECTION` is set, even with an explicit `PULSE_SERVER` (the
environment override is applied after the lookup). Remove `SSH_CONNECTION`
from the private compositor's environment: it is a local display, not a
forwarded one. Changing ECCE's environment later cannot break this cycle.

The display fixture must require a successful readiness probe before
launching services or apps. A timed-out probe must report the compositor log
instead of proceeding to unrelated empty-trace failures.
