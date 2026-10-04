---
type: rule
title: "TCP broker accounts are the data server logins"
area: services
section: "Pitfalls"
paths: ["src/util/jms/MqttLink.C", "src/tdat/resources/AuthCache.C", "src/apps/gateway/GatewayApp.C", "packaging/gateway/ecce-mosquitto.acl", "packaging/dataserver/ecce-dataserver-adduser", "packaging/gateway/ecce-broker-setup"]
issues: [194, 213]
---
**A TCP broker (central or shared) takes the data server account name and
password; the account name is the topic user `ecce/<name>/`.** The ACL is
`pattern readwrite ecce/%u/#` plus `pattern read ecce/+/ecce_machreg_changed`
-- `pattern`, not `topic`: a `topic` line above the first `user` line applies
to anonymous clients only. Things that fail silently:

- **Credential source.** `MqttLink` asks a provider that `AuthCache.C`
  registers at static-init time (util cannot call tdat): the entry of the
  session store (`ECCE_SERVER_LOGIN`'s, else the last). With no login yet
  `ensureConnected()` returns false and retries on the next use, so the
  gateway must log in *before* its first subscribe/publish --
  `GatewayApp::OnInit` subscribes after `checkUser()` for that reason.
  The broker file's `user=` is ignored for TCP. A Unix-socket broker is
  anonymous and needs none.
- **Accounts.** `ecce-dataserver-adduser` writes the hashed password to
  `.ECCE/dataserver/mosquitto_passwd` (`mosquitto_passwd -U` on a private
  temp file; `-b` would put it on a command line) and SIGHUPs the account's
  broker. Accounts made before this have no entry and cannot be added
  without their password. Mode 3: `ecce-broker-setup --user NAME`
  (`siteconfig/SharedBroker.passwd`, then `systemctl reload ecce-broker`).
  Apache's `users` hashes are unreadable to mosquitto, so there are two
  files written by one command.
- **A refused login** is reported once on stderr with the reason code and
  the account, then retried quietly; the wrapper's terminal shows it.
- **Delivery, not SUBACK, is the test.** Mosquitto grants a wildcard
  subscription to a user the read rule then withholds everything from;
  `ctest -R mqtt-auth` checks delivery.
- **Receivers must not trust payloads.** Another account may publish to
  `ecce_machreg_changed`; `JMSMessage::loadBody` ends the process on a
  malformed body, so `MqttEndpoint::process` validates first.
