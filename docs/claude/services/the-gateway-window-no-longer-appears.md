---
type: map
title: "The Gateway window no longer appears"
area: services
section: "The Gateway window and session end"
paths: ["~/.ECCE/mosquitto.server", "packaging/dataserver/ecce-dataserver-stop", "packaging/gateway/ecce-gateway-reap", "src/apps/organizer/CalcMgr.C", "src/apps/gateway/Gateway.C"]
issues: [93, 97, 185, 191, 233]
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
session; a server's broker never does on a plain quit**
(#191). "A server's" comes only from declarations, never from who is
connected (clients may tunnel in over loopback): `siteconfig/
SharedBroker` (mode 3, a systemd service no user can stop, not even
with Quit and Stop Server), or `~/.ECCE/mosquitto.server` (`ecce-remote-
setup --server`; mode 2, stopped only by that account's Quit and Stop
Server). The per-user broker is `mosquitto` on a Unix socket (#213). The data server is
stopped only by Quit and Stop Server, in every mode (#97). **Quit and
Stop Server stops nothing while another session of the account (any
display) or a job is running** (#233): the Quit dialogs do not offer it
then (`SessionLease::othersUsingServices`), and `ecce-dataserver-stop
--if-unused` and `ecce-gateway-reap --stop` refuse
(`ecce_others_using_services`). Without that, the second of two `ecce`
on one display took the first one's broker and data server down.
