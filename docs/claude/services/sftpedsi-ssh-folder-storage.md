---
type: pitfall
title: "SftpEDSI (sftp:// URLs): FileEDSI's logic on a scratch mirror, server-side mkdir lock, and the link and test traps"
area: services
paths: ["src/dsm/edsiimpl/SftpEDSI.C", "include/dsm/SftpEDSI.H", "src/dsm/edsiimpl/EDSIFactory.C", "tests/transport/testSftpEDSI.C", "tests/transport/sshd/sftpedsi_test.sh", "tests/filedsi/run_tests.py"]
issues: [767]
---
**How it works.** `sftp://user@host/folder` is an SSH folder with FileEDSI's
layout. Every operation runs sh scripts through `RCommand::execout` (and
`copyOnLogin` for file content), so it uses the OpenSSH master or libssh
exactly as jobs do; no SFTP API is used and no transport code changed. The
metadata logic is not rewritten: one script lists the directory (stubs with
the real size and mtime, plus the real `.ecce-meta` of it and of each
subdirectory, base64 on one line), the stubs go into a scratch directory,
a `FileEDSI` runs the operation there, and the difference (changed
sidecars, new or removed entries) is sent back as one script. Move and copy
are `mv`/`cp -R` on the server; FileEDSI on the mirror only works out the
sidecar changes. Listing is one round trip.

**Lock.** `<dir>/.ecce-meta.lockdir`, made with atomic `mkdir`, held over the
whole read-modify-write and released by the destructor. A lock older than
two minutes is taken as left by a dead client (broken by `mv` then `rmdir`;
two clients breaking the same stale lock at once can still race). FileEDSI's
`flock` file is not used on the server. Data puts do not lock: they upload
to a hidden temp name and `mv` over the target.

**Traps found.**
- `execout` turns every `\n` into `\r\n` and merges stderr, so nothing may be
  framed by byte count (sidecars travel as base64) and `\r` is stripped.
- A script's exit status is its last command: a trailing `[ ... ] && cmd`
  that is false made `execout` report failure. Scripts end in `true`.
- `RCommand` has no port argument: a `:port` in the URL is dropped and the
  port comes from the ssh configuration.
- `eccedsi` now needs `eccercmd` (and libssh) at link time:
  `target_link_libraries(eccedsi PUBLIC eccercmd)`, and `-lssh` in
  `tests/filedsi/run_tests.py`; a hand-written link line of any test that
  uses `eccedsi` statically needs the same.
- Tests run in the build container because OpenSSH reads `~/.ssh/config`
  from the passwd home, not `$HOME`; the program needs `ECCE_HOME`,
  `ECCE_REALUSER` and `ECCE_REALUSERHOME` set.
