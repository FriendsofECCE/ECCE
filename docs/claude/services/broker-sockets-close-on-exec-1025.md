---
type: pitfall
title: "Every socket in a process that starts programs needs close-on-exec: system() and execv() pass all descriptors on, and a child holding the broker's end hides EOF from its clients"
area: services
section: "Pitfalls"
paths: ["src/util/jms/MqttBroker.C", "src/util/jms/MqttLink.C", "src/apps/organizer/Launch.C", "src/wxgui/EcceShell.C", "tests/mqtt/mqtt_test.C"]
issues: [1025]
---
glibc `system()` and a plain `fork()`+`execv()` close no descriptors, so a
child gets every socket its parent has that lacks `FD_CLOEXEC`.
`Launch::startJobStore` starts eccejobmaster with `system("nohup ... &")`
and `EcceShell::spawnDetached` dup2s only 0-2 before `execv`. When the broker
ran in the same process with inheritable sockets, eccejobmaster and terminal
windows kept its listening socket and every accepted client open. After
the host process exited, clients (job programs, `ecce-*` commands) never
saw EOF and hung instead of reconnecting.

`MqttBroker` therefore creates its sockets through `newSocket()` and
`acceptSocket()` (`SOCK_CLOEXEC` and `accept4` on Linux, `FD_CLOEXEC` on
macOS, `HANDLE_FLAG_INHERIT` cleared on Windows). `MqttLink::onConnect` sets
the same on libmosquitto's socket, on every reconnect, and `MqttLink::ensureConnected` marks
the socketpair `mosquitto_new()` opens (libmosquitto 2.0 sets no flag on
either). The wake-up pipe
(`MqttEndpoint::enablePipe`) already had it. A new `socket()`, `accept()`,
`pipe()` or `open()` that stays open in the Organizer or a long-lived app
needs the flag too. `DirectTransport` children are safe either way: they
call `closeFrom(3)` before exec.

Test: `mqtt-builtin-cloexec` (mqtt_test cloexec) starts the session broker,
a TCP broker and a libmosquitto client, then lists `/proc/self/fd` in a
`system()` child and expects none of their socket inodes. It also checks
that a deliberately inheritable socket does reach the child, so the check
can detect a leak.
