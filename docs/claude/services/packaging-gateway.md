---
type: map
title: "`packaging/gateway/`"
area: services
section: "The two background services (\"the server\")"
paths: ["java/", "packaging/gateway/"]
---
**`packaging/gateway/`** — the per-user message broker (#213). There is
no relay process: `ecce-gateway-start` starts (or reuses) this user's
`mosquitto` on a Unix socket `$STATEDIR/mosquitto.sock` (dir 0700, no TCP,
no auth; config `$STATEDIR/mosquitto.conf` is regenerated, log
`mosquitto.log`, pid `mosquitto.pid`) and writes `$STATEDIR/broker`, which
every ECCE process reads (`MqttLink.C`): `socket=` for the local broker,
`host=`/`port=` for the central (`-remote`, host and port from
`siteconfig/jndi.properties`) or shared (`SharedBroker`) one, plus
`user=` (topic user). A deep `$STATEDIR` (socket path over 100 bytes)
moves the socket into a 0700 directory under `$XDG_RUNTIME_DIR`.
Session-class messages go through the broker under
`ecce/<user>/session/<host>_<display>/`. `ECCE_BROKER_PORT` no longer
means anything in local mode. Remote and shared paths use plain TCP
until stage 4 (auth/TLS); the server-side mosquitto (`ecce-remote-setup
--server`, `ecce-broker.service`) is stage 3 and the `remote`, `shared`
and `markers` cases of `tests/apps/session_end.py` skip until it exists.
