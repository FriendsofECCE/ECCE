---
type: pitfall
title: "SSH-folder trees in the Organizer (#767 b2): keyed by address, prompts only while connecting, a port is part of the address"
area: services
paths: ["src/apps/organizer/CalcMgr.C", "src/wxgui/wxtools/WxResourceTreeCtrl.C", "src/dsm/edsiimpl/SftpEDSI.C", "src/util/genutil/ServerList.C", "src/wxgui/wxdialogs/AddServerDialog.C", "tests/launch/ssh_folder_test.sh"]
issues: [767, 789]
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
- Port (#789): the address carries it (`sftp://user@host:2222/folder`, `:22`
  never written, so old addresses and keys are unchanged); `parseSsh` returns
  the authority `host[:port]` and `ServerList::splitHostPort` splits it.
  `SftpEDSI` keeps the authority as its host (login map key, messages) and
  splits only where it makes the `RCommand` (its `port` argument). RCommand
  passes it to both transports (owned master socket hash and SecretStore key
  include it) and keys AuthCache as `ssh://host:port` (`passHost`), which
  `makeKey`'s getRef + "/" keeps from prefix-matching `ssh://host/` under
  BEST_URL. A non-zero port is always remote (`isRemote` ignores ports).
- Tests: OpenSSH reads ~/.ssh from the passwd home, so `ssh_folder_test.sh`
  binds a private ~/.ssh in `unshare --user --mount --map-user --keep-caps`; the
  real one is untouched. `ECCE_SFTP_NOLOCK=1` (test only) disables the server
  lock; `sftpedsi_test.sh` uses it as a negative control.
