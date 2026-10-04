---
type: map
title: "`packaging/gateway/`"
area: services
section: "The two background services (\"the server\")"
paths: ["packaging/gateway/"]
---
**`packaging/gateway/`** — the per-user message broker (#213). There is
no relay process: `ecce-gateway-start` starts (or reuses) this user's
`mosquitto` on a Unix socket `$STATEDIR/mosquitto.sock` (dir 0700, no TCP,
no auth; config `$STATEDIR/mosquitto.conf` is regenerated, log
`mosquitto.log`, pid `mosquitto.pid`) and writes `$STATEDIR/broker_<host>_<display>` (name sanitised by
`tr -c 'A-Za-z0-9._-' _`, in C++ too), which every ECCE process reads
(`MqttLink.C`): `socket=` for the local broker, `host=`/`port=` for the
central (`-remote`: the host of `siteconfig/RemoteServer/DataServers`,
port `ECCE_BROKER_PORT`, default 8883) or shared (`SharedBroker`) one,
plus `user=` (topic user). The reaper removes the files of displays with
no app left (not younger than a minute: the start script writes before
the app starts) and all of them when it stops the broker; `ecce-gateway-
stop` removes its display's. A deep `$STATEDIR` (socket path over 100 bytes)
moves the socket into a 0700 directory under `$XDG_RUNTIME_DIR`.
Session-class messages go through the broker under
`ecce/<user>/session/<host>_<display>/`. `ECCE_BROKER_PORT` means
nothing in local mode. Remote and shared paths use plain TCP until stage 4
(auth/TLS); the server-side mosquitto (`ecce-remote-setup --server`,
`ecce-broker-setup`, `ecce-broker.service`) is stage 4 and the `remote`,
`shared` and `markers` cases of `tests/apps/session_end.py` skip until it exists. Packages: ecce-client depends on `libmosquitto1` and
recommends `mosquitto` (the gateway scripts ship there); ecce-server
depends on `mosquitto`; RPM `mosquitto` covers both.
