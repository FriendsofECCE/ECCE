---
type: map
title: "Local sessions run ECCE's own broker: in the Organizer on Windows, as eccebroker on Linux and macOS"
area: services
paths: ["include/util/MqttBroker.H", "src/util/jms/MqttBroker.C", "src/apps/organizer/CalcMgrApp.C", "packaging/windows/ecce.cmd", "packaging/gateway/ecce-gateway-start", "packaging/gateway/ecce-gateway-status", "packaging/windows/ecce-broker-win", "tests/mqtt/mqtt_test.C", "tests/mqtt/broker_cli.C", "tests/apps/session_end.py"]
issues: [990, 1007, 133, 958, 973, 522]
---
Windows start failures kept being broker failures (SAC blocking mosquitto's
DLL, slow first start, antivirus holding a new program). A local Windows
session now has no broker program: `MqttBroker` (plain C++17 + Winsock/POSIX
sockets, in ecceutil, wx-free) runs in the Organizer.

- **Start.** `CalcMgrApp::OnInit` calls `MqttBroker::startForSession()` first
  of all when `ECCE_LOCAL_BROKER` is set (`ecce.cmd`, local session). It makes
  a login, listens on 127.0.0.1 at a free port, and writes the broker file
  (`user=`, `password=`, `host=`, `port=`, same format) and the session
  pointer, atomically. `ecce-gateway-start` on Windows returns at once;
  `OnExit` stops the broker and removes the file.
- **Session end.** At exit the Organizer calls `lingerForSession(10)`: it
  disconnects its own link, and if Builder, Calculation Editor or job
  stores are still connected it stays alive without a window, serving the
  broker, until no client has been connected for 10 s; then it removes the
  broker file. No client left: it stops at once.
- **Linux and macOS (#1007).** Not in a GUI process: the gateway connects
  (it is the app execer) before the Organizer exists, `ecce` waits for the
  gateway, and a job store that outlives the session must keep its broker
  as it did with the per-user mosquitto. `ecce-gateway-start`, in the
  mode-1 branch, runs `$ECCE_HOME/libexec/eccebroker --serve
  ${ECCE_BROKER_LINGER:-10}`: it forks before any thread, the child writes
  the broker file (`user=local`, `socket=~/.ECCE/mqtt-<hex>.sock`, or a
  0700 dir under `$XDG_RUNTIME_DIR` when the path would pass ~100 bytes)
  and the parent prints "started" / "already running" (a session whose
  socket answers is adopted). It ends when no client has been connected
  for the grace time (60 s before the first), when its broker file no
  longer names its socket (`ecce-gateway-stop`, the reap sweep), or on
  SIGTERM. **libexec, not bin**: a program in `$ECCE_HOME/bin` with a
  session id counts as an open window in `ecce_session_procs`, the
  gateway's `otherSessionApps` and `SessionLease`, and would keep every
  session alive. The socket listener takes any CONNECT and applies no
  topic rules, the rule of the per-user mosquitto's socket
  (`allow_anonymous true`, no `acl_file`): the 0700 directory and the
  0600 socket keep other accounts out.
- **Still mosquitto.** A server account (`~/.ECCE/mosquitto.server`, or the
  8.x `activemq/server` mark): its broker also serves the clients over TCP.
  `-remote` and `siteconfig/SharedBroker` are decided before the mode-1
  branch and never reach it. `ECCE_BROKER=mosquitto`, or no
  `libexec/eccebroker` in the install, keeps the old per-user route.
- **Tests.** `tests/apps/session_end.py` runs its mode-1 cases with
  `ECCE_BROKER=mosquitto` by default: they check the per-user mosquitto
  (pid file, one broker shared by an account's sessions), which the
  built-in broker is not. The launch harnesses link `eccebroker` into
  their tree's `libexec` and set `ECCE_BROKER_LINGER=900`, since no window
  holds the broker open between their jobs.
- **Escape hatch.** `ECCE_BROKER=mosquitto` in the environment of `ecce.cmd`
  runs the old path (`ecce-broker-win` + a mosquitto installed on the machine
  and found by `ecce-find-mosquitto`; mosquitto.exe is no longer bundled).
- **Subset.** MQTT 5 only (3.1.1 gets CONNACK 1). CONNECT with user/password
  (properties and will read and ignored), SUBSCRIBE/UNSUBSCRIBE with `+`/`#`
  and the no-local option, PUBLISH QoS 0/1 with properties forwarded verbatim,
  PING, DISCONNECT. No retain, no will, no persistent sessions, no QoS 2
  (CONNACK advertises Maximum QoS 1 / Retain unavailable; an inbound QoS 2 gets
  DISCONNECT 0x9B). MqttLink uses subscribe QoS 1, publish QoS 1, retain false,
  keepalive 30, clean start, random client id.
- **ACL** as `packaging/gateway/ecce-mosquitto.acl`: read+write `ecce/<user>/#`,
  read `ecce/+/ecce_machreg_changed`. Like mosquitto, a subscription is
  granted and delivery is withheld; a refused QoS 1 publish gets PUBACK 0x87.
- **Threading.** One thread, `poll`/`WSAPoll` with a 100 ms timeout that also
  checks the stop flag; non-blocking sockets; per-client output buffer
  (64 MiB cap, then the client is dropped). Keepalive 1.5x, CONNECT within 10 s.
- **Unit tests.** `ctest -R mqtt-builtin`: `-socket` (anonymous CONNECT,
  no topic rules, 0600), `-serve` (eccebroker --serve: adopt, linger,
  ends when its file goes), `-auth` (the mosquitto auth test's checks
  against MqttBroker plus ping, keepalive, malformed input, 3000 messages),
  `-session` (startForSession and the broker file, MqttLink with the file's
  login), `-interop` (mosquitto_pub/sub -V 5; skipped without them).
  `eccebroker` (tests/mqtt/broker_cli.C) runs it as a program.
