---
type: pitfall
title: "AuthCache::pipeIn removes only a FIFO or ECCE's own AuthPipe.* temp file, never a path the caller passed"
area: services
paths: ["src/tdat/resources/AuthCache.C", "tests/mqtt/authcache_pipe_test.C", "tests/apps/fixture.py", "tests/launch/harness.py"]
issues: [1060, 1047]
---
`pipeIn` and `pipeClean` used to unlink any `-pipe` path after reading, so
`launchjob -pipe /dev/null` run as root removed /dev/null. They now `lstat`
the path and unlink only:

- a FIFO (what `pipeName()` + `pipeOutData()` make on Linux/macOS: Launch.C,
  RCommand.C, eccejobmaster.C);
- a regular file named `AuthPipe.*` that is the Windows form of the same
  handoff. On POSIX it must also be in `TempStorage::getTempRootPath()`
  (same dev/inode), owned by the euid, mode without group/other bits and
  one link; Windows has no inode or owner, so the name alone decides there.

Symlinks, devices, directories and any other file are left. The test
harnesses (`fixture.authFile`, `harness.authFile`) write regular 0600 files
into their own state directory; those stay until the state directory goes.
A new caller that writes credentials to a regular file must either use
`pipeName()`/`pipeOutData()` or delete the file itself.
