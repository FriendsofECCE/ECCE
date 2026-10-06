---
type: map
title: "The session id (`ECCE_SESSION_ID`) and session liveness"
area: services
section: "The Gateway window and session end"
paths: ["packaging/gateway/ecce-session-lib.sh", "packaging/ecce.in", "src/util/genutil/Ecce.C", "src/util/jms/MqttLink.C", "src/apps/gateway/GatewayApp.C", "packaging/gateway/ecce-gateway-reap", "packaging/gateway/ecce-gateway-stop", "tests/session", "tests/apps/session_end.py"]
issues: [233, 186, 133]
---
**A session is `ECCE_SESSION_ID`, not `DISPLAY`** (#233). 16 lower-case
hex characters. Every `ecce` makes a new one (two `ecce` on one display
are two sessions); every program it or the gateway starts inherits it,
whatever its `DISPLAY` (an `ssh -X` reconnect stays one session).

- **The key** is `<host>_<id>`, host = `ECCE_HOST`, else `HOST`, else
  `hostname`, every byte outside `[A-Za-z0-9._-]` made `_`; a malformed id
  gives no key. It names `~/.ECCE/broker_<key>`, `authcache_<key>` and the
  topic level `ecce/<user>/session/<key>/`. Derived in exactly two places,
  `ecce-session-lib.sh` (`ecce_session_key`) and `Ecce::sessionKey()`;
  `ctest -R session-key` checks they agree. Do not add a third.
- **Without an id.** An `ecce-<app>` wrapper joins the id in
  `~/.ECCE/session_<host>` (the newest session, written by
  `ecce-gateway-start` with a session's first broker file) while an ECCE
  program of it is alive, else makes one. A C++ process without one
  (MqttLink: a job store started over ssh) joins the pointed-to session
  while its broker file exists, and otherwise has no messaging, quietly.
- **Liveness** -- the hidden gateway's session end
  (`GatewayApp::otherSessionApps`), which gateway `ecce-gateway-stop`
  kills, and which files the reaper keeps -- is "an ECCE program with this
  id is running". An ECCE program is one whose resolved executable is
  `$ECCE_HOME/bin`'s (or what `bin/<name>` links to). `eccejobstore` counts
  for the reaper (a running job keeps its session's broker file and
  credential, decision 6) but not for session end.
- **`DISPLAY`** is only X: `ctest -R session-display-gate` fails on a
  `getenv("DISPLAY")` outside the allowed files or any use in
  `packaging/gateway`. The wrappers still default it to `:0` for X
  programs under cron or ssh, until #166.
- Files of the DISPLAY-keyed 9.0.0-alpha scheme match no live id and are
  swept on the first start.
