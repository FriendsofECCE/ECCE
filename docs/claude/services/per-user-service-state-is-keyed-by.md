---
type: pitfall
title: "Per-user service state is keyed by `$DISPLAY`, and the \"is it already running?\" checks were not"
area: services
section: "Pitfalls"
paths: ["$STATEDIR/broker", "$STATEDIR/mosquitto.pid", "httpd.conf.ecce", "tests/apps"]
---
**Per-user service state is keyed by `$DISPLAY`, and the "is it
already running?" checks were not.** (Written for the JMSDispatcher
relay, which #213 removed; the lesson stands.) The relay was per
*session*, but its pidfile was a single display-agnostic file, so a
second display printed "already running", never wrote its own port
file, and every app aborted. What is per display now is only the
session credential file (`authcache_<host>_<display>`) and the session
topics (`ecce/<user>/session/<host>_<display>/`); the broker is per user
(`mosquitto.pid`, `broker`) and the reaper decides its lifetime from the
process table, not from any per-display marker. Same shape as the
`httpd.conf.ecce` gotcha: a start script that early-exits when it
thinks the service is up, while what it needs is per-session. Found by
`tests/apps` on its first run -- no amount of single-desktop manual
testing surfaces it.
