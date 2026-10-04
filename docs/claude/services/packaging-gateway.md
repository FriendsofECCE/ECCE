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
port `ECCE_BROKER_PORT`, default 8088) or shared (`SharedBroker`) one,
plus `user=` (topic user). The reaper removes the files of displays with
no app left (not younger than a minute: the start script writes before
the app starts) and all of them when it stops the broker; `ecce-gateway-
stop` removes its display's. A deep `$STATEDIR` (socket path over 100 bytes)
moves the socket into a 0700 directory under `$XDG_RUNTIME_DIR`.
Session-class messages go through the broker under
`ecce/<user>/session/<host>_<display>/`. `ECCE_BROKER_PORT` means
nothing in local mode (it is where a server account's broker listens and
where `-remote` clients look). TCP brokers (central, shared) are plain TCP
(TLS later) and authenticated: see
[broker accounts](broker-accounts-are-the-data-server-logins.md). A server
account (`~/.ECCE/mosquitto.server`, `ecce-remote-setup --server`) gets
the same mosquitto as mode 1 plus TCP listeners (the data server's listen
setting) with `per_listener_settings`: socket anonymous for its own
processes, TCP with the `server/ecce_users_auth.so` plugin
(logins checked against `dataserver/users`) and `server/mosquitto.acl`; `ecce-gateway-start` restarts a broker whose
generated `mosquitto.conf` changed. Mode 3: `ecce-broker.service` runs
`ecce-broker-run --shared <base>` (generated conf, accounts copied from
`siteconfig/SharedBroker.passwd` by root `ExecStartPre=+`/`ExecReload=+`;
never run under the system manager yet). Debian's own `mosquitto.service`
(1883) is neither used nor disturbed. Packages: ecce-client depends on
`libmosquitto1` and recommends `mosquitto` (the gateway scripts ship
there); ecce-server depends on `mosquitto` and `libaprutil1` (RPM `apr-util`: the plugin); RPM `mosquitto` covers both.
`tests/apps/session_end.py` covers all cases on tree binaries; `ctest -R
mqtt` checks delivery on both a Unix-socket and a two-account TCP broker.
