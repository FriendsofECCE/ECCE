---
type: pitfall
title: "subscribe() skips a process's own messages by default (MQTT No Local): a topic the same app also publishes needs filterSelf=false"
area: services
section: "Pitfalls"
paths: ["src/util/jms/JMSSubscriber.C", "src/util/jms/MqttLink.C", "include/util/JMSSubscriber.H", "include/wxgui/WxJMSSubscriber.H", "src/apps/organizer/CalcMgrApp.C"]
issues: [213]
---
`JMSSubscriber::subscribe(topic, handler, filterSelf = true)`: with the
default, `MqttLink::syncSubscriptions` subscribes with
`MQTT_SUB_OPT_NO_LOCAL`, and the endpoint drops what its own process sent.
One `MqttLink` serves the whole process, so every publisher in it counts as
"self", including a dialog with its own `JMSPublisher` (GlobalPrefs).
Nothing is logged; the handler just never runs for that sender.

Seen with Preferences > Colours > Run-state icons: GlobalPrefs, opened from
the Organizer, published `ecce_preferences_misc`, and the Organizer's own
handler (which rebuilds the tree's icon lists) never ran. It now subscribes
with `filterSelf=false`. Any handler meant to follow a change made in the
same app needs the same, or a direct call.
