---
type: rule
title: "The per-user data server starts with no accounts; its login window offers to create one and to reset a password"
area: services
section: "The Gateway window and session end"
paths: ["src/wxgui/wxdialogs/WxDavAuth.C", "src/wxgui/wxdialogs/WxAuth.C", "src/wxgui/wxdialogs/WxAuthGUI.C", "packaging/dataserver/ecce-dataserver-adduser", "packaging/dataserver/ecce-dataserver-start", "tests/apps/session_end.py"]
issues: [203]
---
`ecce-dataserver-start` creates an empty `~/.ECCE/dataserver/users`, so a
fresh user choosing "Connect to a server" > `localhost` (the per-user data
server, #259) could never log in. `WxDavAuth::prompt` now asks, after OK,
"No account yet ... create it with this password?" when this is the
per-user server (`perUserServer()`: no `ECCE_REMOTE_SERVER`, not local
data, `bin/ecce-dataserver-adduser` installed, account file present) and
the name is not in the file, and runs `ecce-dataserver-adduser --stdin NAME`.
The same test shows **Forgot password...**: it asks for the new password
twice and runs `ecce-dataserver-adduser --passwd NAME` (an existing account
only), then logs in with it. The password goes on stdin, never argv (`-b`
shows it in `ps`); the tool's stderr is shown on failure. A central server
(`ECCE_REMOTE_SERVER`) is never offered either: its accounts are the
administrator's, and a refused login there names the administrator's command
`ecce-dataserver-adduser --passwd NAME`. The per-user broker is a Unix socket
and anonymous, so there is no second password list.
Test hooks: `ECCE_TEST_AUTH_ANSWER=login:USER:PASS` fills the login window
and answers Yes; `=reset:USER:PASS` presses Forgot password... once and
enters PASS; both log `reset offered|not offered` and a refused login's
status line. `session_end.py create-account create-account-central`.
