---
type: pitfall
title: "Per-session state is keyed by the session id, and the \"is it already running?\" checks must be too"
area: services
section: "Pitfalls"
paths: ["$STATEDIR/broker_<host>_<id>", "$STATEDIR/mosquitto.pid", "httpd.conf.ecce", "tests/apps"]
issues: [233]
---
**Per-session state is keyed by the session, and a check that something
is "already running" must use the same key.** (Found with the
JMSDispatcher relay, which #213 removed; the lesson stands.) The relay
was per *session*, but its pidfile was a single session-agnostic file,
so a second session printed "already running", never wrote its own port
file, and every app aborted. What is per session now is the credential
file (`authcache_<host>_<id>`), the broker file (`broker_<host>_<id>`,
so a local and a `-remote` session of one account do not overwrite each
other) and the session topics (`ecce/<user>/session/<host>_<id>/`); the
key is `<host>_<ECCE_SESSION_ID>` (see
[the session id](the-session-id.md)). Until #233 the key was
`<host>_<DISPLAY>`, so two `ecce` on one display shared one session and
an `ssh -X` reconnect started another. The broker itself is per user
(`mosquitto.pid`) and the reaper decides its lifetime from which ECCE
programs are alive, not from any per-session marker. Same shape as the
`httpd.conf.ecce` gotcha: a start script that early-exits when it thinks
the service is up, while what it needs is per-session. Found by
`tests/apps` on its first run -- no amount of single-desktop manual
testing surfaces it.
