---
type: pitfall
title: "mosquitto_pub can exit zero after a refused MQTT 5 publish"
area: services
section: "Pitfalls"
paths: ["tests/mqtt/mqtt_test.C", "src/util/jms/MqttBroker.C"]
issues: [1027]
---
`mosquitto_pub` 2.0.21 prints a warning for a negative MQTT 5 PUBACK but
still exits zero after its orderly disconnect. Its process status is not
the publish authorization result. `MqttBroker::onPublish` correctly sends
PUBACK `0x87` (135, Not authorized) and returns before forwarding a publish
outside the authenticated account's `ecce/<user>/` tree. The
`mqtt-builtin-auth` test checks that reason code through libmosquitto.

`mqtt-builtin-interop` must inspect `mosquitto_pub -d` output for the
received PUBACK's `RC:135`, rather than require a nonzero process status.
It also requires that the forbidden payload never reaches the subscriber.
The subscriber must remain connected through the forbidden publish: send
the final permitted machine-registration message afterwards, and require
that later message to arrive before accepting the absence check. Ending
`mosquitto_sub -C 2` on two permitted messages sent before the attack would
leave the actual attempted cross-user delivery unobserved.

The relevant upstream client behavior is in
[`client/pub_client.c`, `my_publish_callback` and `pub_other_loop`](https://github.com/eclipse-mosquitto/mosquitto/blob/v2.0.21/client/pub_client.c).
The debug PUBACK reason code is emitted by
[`lib/handle_pubackcomp.c`](https://github.com/eclipse-mosquitto/mosquitto/blob/v2.0.21/lib/handle_pubackcomp.c).
