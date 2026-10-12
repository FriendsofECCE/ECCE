---
type: rule
title: "The per-user data server starts with no accounts; its login window offers to create one"
area: services
section: "The Gateway window and session end"
paths: ["src/wxgui/wxdialogs/WxDavAuth.C", "packaging/dataserver/ecce-dataserver-adduser", "packaging/dataserver/ecce-dataserver-start", "tests/apps/session_end.py"]
issues: [203]
---
`ecce-dataserver-start` creates an empty `~/.ECCE/dataserver/users`, so a
fresh user choosing "Connect to a server" > `localhost` (the per-user data
server, #259) could never log in. `WxDavAuth::prompt` now asks, after OK,
"No account yet ... create it with this password?" when this is the
per-user server (no `ECCE_REMOTE_SERVER`, not local data, `bin/ecce-dataserver-adduser`
installed, account file present without the name) and runs
`ecce-dataserver-adduser --stdin NAME`. The password goes on stdin, never
argv (`-b` shows it in `ps`). A central server (`ECCE_REMOTE_SERVER`) is
never offered this: its accounts are the administrator's. The per-user
broker is a Unix socket and anonymous, so there is no second password list.
Test hook: `ECCE_TEST_AUTH_ANSWER=login:USER:PASS` fills the login window and
answers Yes; `session_end.py create-account create-account-central`.
