---
type: pitfall
title: "An adopted session broker ended before the adopting app connected; the app aborted on the missing broker file (#1040)"
area: services
section: "Pitfalls"
paths: ["src/util/jms/MqttBroker.C", "tests/mqtt/broker_cli.C", "packaging/gateway/ecce-gateway-start", "src/util/jms/MqttLink.C", "src/wxgui/jms/WxJMSSubscriber.C", "tests/apps"]
issues: [1040, 1007, 233]
---
**`eccebroker --serve` adopts a session broker whose socket answers, and
that broker may be in its last seconds.** Every `ecce-<app>` wrapper runs
`ecce-gateway-start`, which runs `eccebroker --serve`; with the session's
broker still answering, the new process exits ("already running"). The
hosting broker had no client and was counting down its 10 s grace; the
app's first connection comes seconds later (wx start-up, loading), so the
broker could end and remove `broker_<host>_<id>` in between. In a user
session the gateway is a client for the whole session and this cannot
happen; it happens when apps run one after another with no gateway (the
`tests/apps` suite, an `ecce-builder` started from a shell), and more often
on a slow machine.

- **The broker counts accepted connections** (`MqttBroker::connections()`);
  the adopter's probe (`socketAnswers`) is one. A connection that is not a
  client restarts the wait as for a first client (60 s), not the grace.
- **A missing or empty broker file is not fatal** in `MqttLink`: stderr
  says so, the process runs without messaging (`sessionless()`, as one
  started outside any session), and a GUI program shows one warning box
  (`MqttLink::setNoBrokerHandler`, installed by `WxJMSSubscriber`; not
  under `ECCE_TEST_HOME`, where a modal box would stall a scripted app).
  Before, `EE_RT_ASSERT` aborted the app and the user lost the window.
