---
type: map
title: "`ecce -admin -remote` saves on the central server over ssh (`ecce-site-admin`), publishes, then refreshes the client's copy"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/apps/machregister/SiteAdminClient.C", "include/tdat/SiteRequest.H", "src/tdat/resources/SiteRequest.C", "src/apps/siteadmin/ecce-site-admin.C", "packaging/dataserver/ecce-site-publish", "packaging/dataserver/ecce-remote-setup", "packaging/dataserver/ecce-dataserver-start", "tests/queues/siteadmin_test.py", "tests/queues/central_server.py"]
issues: ["234", "188"]
---
On a `-remote` client, `siteconfig/` holds the server's published copy and is
overwritten by `ecce-remote-setup`, so Register Machines in admin mode never
writes it. Save, Delete and "Advanced: edit file" build a `SiteRequest` (the
processmachine form plus the draft's `ConfigEdit`s, or the raw text and the
text it was edited from) and `SiteAdminClient::send` runs it on the server:

1. `RCommand(host, "ssh", "bash", $ECCE_SITE_ADMIN_LOGIN)` - the job
   transport, so ControlMaster, askpass and host-key dialogs behave as for
   jobs. The host is the one in `siteconfig/RemoteServer/DataServers`.
2. `mktemp -d` there, `cd`, `shellput` the request file, then the fixed
   command `ecce-site-admin apply request`. No user text is ever in a command;
   names and values are percent-encoded inside the file.
3. On the server, `ecce-site-admin` (from PATH; the `/usr/bin` wrapper sets
   `ECCE_HOME`) checks that the login may write `siteconfig` - its group's
   write bit and membership are the authorisation, the data-server password is
   not involved - runs processmachine, writes `CONFIG.<m>` through
   `ConfigFile`, and runs `$ECCE_HOME/bin/ecce-site-publish`.
4. The client runs `ecce-remote-setup --refresh` when it can write its
   `siteconfig`; otherwise it says to run it with sudo. Then `redo()` reloads.

Things that are easy to get wrong:

- `ecce-site-publish` updates the published directory **in place** (it used
  to be `rm -rf` and recreated by `ecce-dataserver-start`), so a directory
  made group-writable for administrators keeps its group and mode.
  Without an argument it publishes to `siteconfig/PublishDir` (one path) or,
  if absent, to the caller's own `~/.ECCE/dataserver/htdocs`; an
  administrator who is not the data server's account needs `PublishDir`.
- processmachine rewrites `Machines`, `Queues` and `<m>.Q` in place, so they
  must be group-writable, not only the directory; `ecce-site-admin` checks
  them first and sets `umask 002`.
- A raw edit is refused when the server's file no longer reads as the
  client's copy (someone else changed it); per-key edits merge like
  processmachine does.
- `--refresh` removes a `CONFIG.*`/`*.Q` that its previous fetch brought and
  the server no longer publishes (it keeps the last MANIFEST in
  `siteconfig/RemoteServer/MANIFEST`), so a deleted machine disappears.
- Tests run the "server" on this machine with no ssh: host `127.0.0.1` and no
  login make `RCommand` use `DirectTransport`, and an `ecce-site-admin`
  wrapper on PATH switches to the server's `ECCE_HOME`/`ECCE_REALUSERHOME`.
  The real-ssh leg (OpenSSH/libssh `put`, the server's login PATH) is not
  covered by them.
