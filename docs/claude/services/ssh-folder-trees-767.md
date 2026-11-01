---
type: pitfall
title: "SSH-folder trees in the Organizer (#767 b2): keyed by address, prompts only while connecting, a port is refused"
area: services
paths: ["src/apps/organizer/CalcMgr.C", "src/wxgui/wxtools/WxResourceTreeCtrl.C", "src/dsm/edsiimpl/SftpEDSI.C", "src/util/genutil/ServerList.C", "src/wxgui/wxdialogs/AddServerDialog.C", "tests/launch/ssh_folder_test.sh"]
issues: [767]
---
- An `ssh` entry's tree key is its address (`sftp://user@host/folder`), not an
  EcceURL ref; `ServerList::sshRootOf(url)` maps any URL below it to that key,
  `loginFor(address)` is non-empty once `CalcMgr::connectSsh` succeeded.
  `treeOf` uses it, so copies inside one SSH tree are allowed and all others
  keep the old refusals. The root has no `users/<name>` level.
- `SftpEDSI` never prompts unless `probe()` is running (`ECCE_ASKPASS=/bin/false`
  around the RCommand it creates). The job store, monitor and editors are other
  processes: they ride the session's master (OpenSSH) or fail. The Organizer
  therefore connects (and refuses the launch with a message) before submitting.
  With the libssh backend (Windows) the prompt callback is not silenced.
- Port: RCommand and both transports take port 0 from ~/.ssh/config; `host:port`
  in the address is refused by `ServerList::check` ("Use a Host entry in
  ~/.ssh/config ..."), `:22` is accepted and dropped.
- Tests: OpenSSH reads ~/.ssh from the passwd home, so `ssh_folder_test.sh`
  binds a private ~/.ssh in `unshare --user --mount --map-user --keep-caps`; the
  real one is untouched. `ECCE_SFTP_NOLOCK=1` (test only) disables the server
  lock; `sftpedsi_test.sh` uses it as a negative control.
