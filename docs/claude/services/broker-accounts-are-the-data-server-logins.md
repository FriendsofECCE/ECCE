---
type: rule
title: "TCP broker accounts are the data server logins"
area: services
section: "Pitfalls"
paths: ["src/util/jms/MqttLink.C", "src/tdat/resources/AuthCache.C", "src/apps/gateway/GatewayApp.C", "packaging/gateway/ecce-mosquitto.acl", "packaging/dataserver/ecce-dataserver-adduser", "packaging/gateway/ecce-broker-setup", "src/mqttauth/ecce_users_auth.c"]
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
- **Accounts, central server (mode 2).** The broker loads
  `server/ecce_users_auth.so` (`src/mqttauth`, mosquitto plugin API v5,
  `MOSQ_EVT_BASIC_AUTH`) with `plugin_opt_users_file` set to the data
  server's own `dataserver/users`. `apr_password_validate` reads every
  htpasswd hash (bcrypt, apr1 MD5, SHA1, crypt), so there is one account
  list and accounts made by 8.x work unchanged. The file is re-read when
  its mtime, size or inode changes: no restart or SIGHUP after adduser or a
  password change. An unknown user is deferred (refused, as
  `allow_anonymous false` does anonymous clients); a wrong password is
  refused; names containing `/`, `+` or `#` are refused because `%u` is a
  topic level and `a/b` would reach into `a`'s topics. The ACL stays the
  broker's `acl_file`; `ctest -R mqtt-auth-plugin` checks both.
  The plugin needs `libaprutil1`/`apr-util` at runtime and
  `mosquitto-dev` + `libaprutil1-dev` to build.
- **Accounts, shared broker (mode 3).** The per-user data servers sit in
  homes the broker cannot read, so it keeps its own list:
  `ecce-broker-setup --user NAME` (`siteconfig/SharedBroker.passwd`,
  `mosquitto_passwd -U` on a private temp file, then `systemctl reload
  ecce-broker`).
- **A refused login** is reported once per reason on stderr, then retried
  quietly, and the gateway (the first process to log in) also shows a
  dialog "Message broker refused the login" naming the account, once per
  process (`MqttLink::setRefusalHandler`, set in `GatewayApp::OnInit`; util
  has no widgets). Other apps and eccejobstore keep stderr only. Only a
  shared broker can refuse a valid data server login
  (`session_end.py remote-refused`).
- **Delivery, not SUBACK, is the test.** Mosquitto grants a wildcard
  subscription to a user the read rule then withholds everything from;
  `ctest -R mqtt-auth` checks delivery.
- **Receivers must not trust payloads.** Another account may publish to
  `ecce_machreg_changed`; `JMSMessage::loadBody` ends the process on a
  malformed body, so `MqttEndpoint::process` validates first.
