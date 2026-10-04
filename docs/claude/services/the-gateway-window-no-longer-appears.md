---
type: map
title: "The Gateway window no longer appears"
area: services
section: "The Gateway window and session end"
paths: ["~/.ECCE/activemq/server"]
issues: [93, 97, 185, 191]
---
**The Gateway window no longer appears** (#93, 2026-09-22). `ecce`
starts the services and opens the **Organizer** directly, which is now
the front door: the launcher entries live on its File menu (New
Structure) and Tools menu (Register Machines, Machine Browser,
Periodic Table). The Gateway *process* is unchanged and still owns the
session — JMS, service startup, `ecce_get_app`, Quit-and-Stop-Server —
only its frame is hidden, which is one guarded `Show()` call. To get
the old window back for a session: `ECCE_GATEWAY_WINDOW=1 ecce`. If a
UI decision like this needs reversing, prefer that env var to a
revert. With the frame hidden the gateway quits once no other app of
its session is left (#185; job monitors don't count), ending only that
session (there is no relay to stop; the session's credential file goes
with the reaper's sweep). **A per-user broker stops with the user's last
session on any display; a server's broker never does on a plain quit**
(#191). "A server's" comes only from declarations, never from who is
connected (clients may tunnel in over loopback): `siteconfig/
SharedBroker` (mode 3, a systemd service no user can stop, not even
with Quit and Stop Server), or `~/.ECCE/activemq/server` (`ecce-remote-
setup --server`; mode 2, stopped only by that account's Quit and Stop
Server). The per-user broker is `mosquitto` on a Unix socket (#213). The data server is
stopped only by Quit and Stop Server, in every mode (#97).
