---
type: pitfall
title: "A fork()ed child has the parent's broker client but not its loop thread: MqttLink starts over in the child"
area: services
section: "Pitfalls"
paths: ["src/util/jms/MqttLink.C", "include/util/MqttLink.H", "src/comm/commxt/Launch.C", "tests/mqtt/mqtt_test.C"]
issues: [1283, 1284, 1281]
---
`Launch::startJobStore` runs the job store of a single-connection or
front-end machine in a `fork()`ed child (`jobstore_main`), because fork is
the only way to hand an authenticated libssh session to another process.
Over OpenSSH a fresh `eccejobmaster` reuses ECCE's control master instead
(#1281).

`MqttLink::instance()` is a process-wide static. The child got the parent's
`mosquitto` client, connected, with `p_started` set, so `ensureConnected()`
returned true and every publish was queued for a loop thread that exists
only in the parent: the job's state changes never reached the Organizer.
Its exit then called `shutdown()`, which joins that absent thread.

Every `MqttLink` entry point now calls `reclaimAfterFork()`: when `getpid()`
is not the pid that first used the link, it closes the child's copies of
the client's socket and of the socketpair `mosquitto_new` opened, drops
endpoints, subscriptions, config, handlers and local delivery without
destroying anything, and lets the next call connect with a new client id.
Never `mosquitto_disconnect`/`destroy` the inherited client: same client id
(a DISCONNECT ends the parent's session), `destroy` joins the parent's
thread, and a TLS close writes into the parent's stream.

The fork is now only the fallback when eccesshmaster cannot be started
(libssh-connection-master-1284.md); Windows refuses that case before the
submit.

Test: `mqtt-builtin-fork` (`mqtt_test fork`).
