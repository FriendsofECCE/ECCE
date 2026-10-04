---
type: pitfall
title: "Per-user service state is keyed by `$DISPLAY`, and the \"is it already running?\" checks were not"
area: services
section: "Pitfalls"
paths: ["$STATEDIR/${HOST}_${DISPLAY}", "JMSMessage.C:497", "httpd.conf.ecce", "tests/apps"]
---
**Per-user service state is keyed by `$DISPLAY`, and the "is it
already running?" checks were not.** The JMSDispatcher is per
*session*, not per user: `ecce-gateway-start` launches it with a
`-DDISPLAY` and it writes its port to `$STATEDIR/${HOST}_${DISPLAY}`,
which `JMSMessage.C:497` reads back and `EE_RT_ASSERT(EE_FATAL)`s on
when missing. But the pidfile was a single display-agnostic
`jmsdispatcher.pid`, so with a dispatcher already running for one
display, starting ECCE on a **second** display — a VNC session, an
X-forwarded session, a second seat, or a test running under Xvfb —
printed "JMSDispatcher already running", never wrote that display's
port file, and then **every app aborted on SIGABRT**. Fixed by making
the pidfile per-display across `ecce-gateway-{start,stop,status}` and
by requiring the port file to exist too before believing "already
running". Note the shape, because it is the same one as the
`httpd.conf.ecce` gotcha recorded above: a start script that early-
exits when it thinks the service is up, while the thing it actually
needed to create is per-session. Found by `tests/apps` on its first
run, which is exactly the kind of bug no amount of single-desktop
manual testing surfaces.
