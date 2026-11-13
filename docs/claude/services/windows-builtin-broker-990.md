---
type: map
title: "Windows local sessions run ECCE's own broker inside the Organizer"
area: services
paths: ["include/util/MqttBroker.H", "src/util/jms/MqttBroker.C", "src/apps/organizer/CalcMgrApp.C", "packaging/windows/ecce.cmd", "packaging/gateway/ecce-gateway-start", "packaging/windows/ecce-broker-win", "tests/mqtt/mqtt_test.C"]
issues: [990, 133, 958, 973, 522]
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
- **Tests.** `ctest -R mqtt-builtin`: `-auth` (the mosquitto auth test's checks
  against MqttBroker plus ping, keepalive, malformed input, 3000 messages),
  `-session` (startForSession and the broker file, MqttLink with the file's
  login), `-interop` (mosquitto_pub/sub -V 5; skipped without them).
  `eccebroker` (tests/mqtt/broker_cli.C) runs it as a program.
