---
type: map
title: "`packaging/gateway/`"
area: services
section: "The two background services (\"the server\")"
paths: ["java/", "packaging/gateway/"]
---
**`packaging/gateway/`** — ActiveMQ (Debian-packaged, `/usr/share/
activemq`) JMS broker + the `java/` JMSDispatcher relay. How `gateway`
launches other apps (`organizer`, `builder`, ...) and how they publish
progress/results back.
