---
type: map
title: "Second `ecce` start (#1069): the check is in organizer startup, a lock plus a local socket, keyed by user and data folder, not by session id"
area: services
paths: ["src/wxgui/jms/OneInstance.C", "include/wxgui/OneInstance.H", "src/apps/organizer/CalcMgrApp.C", "src/wxgui/jms/MainFrame.C", "packaging/ecce.in", "packaging/windows/ecce-splash", "packaging/windows/ecce.vbs", "packaging/windows/make-msi.ps1", "tests/apps/one_process_test.py"]
issues: [1069, 983]
---
- Where: `CalcMgrApp::OnInit`, right after `takeOneProcessSwitch` and only
  when `ToolTabs::enabled()`, i.e. before the Organizer's own broker is
  started. On Linux and macOS the gateway starts the Organizer, on Windows
  ecce.cmd does; only the Organizer exists on all three, so the check sits
  there. A second launcher therefore still runs the wrapper and (Linux) the
  gateway before the Organizer quits; a later cheap pre-check in the wrapper
  would need a small client program.
- Key: user name, `ECCE_REALUSERHOME`, `ECCE_LOCAL_DATA`,
  `ECCE_SERVER_SESSION`, `ECCE_REMOTE_SERVER`, `ECCE_REMOTE_DIR`. Never
  `ECCE_SESSION_ID`, which is new at every start. A central-server client
  is one copy per Unix account (the lock folder is per uid); one user on two
  servers gets two copies.
- Lock: `wxSingleInstanceChecker` (file lock, released by the OS when the
  process dies, so a crash never locks the user out). IPC: `wxServer`; a Unix
  socket in a private folder (`$XDG_RUNTIME_DIR/ecce-one`, `$TMPDIR/ecce-one`
  on macOS, else `/tmp/ecce-one-<uid>`; checked with lstat to be a directory
  of ours with mode 0700, otherwise the single-instance check is off with a
  line on stderr). The token file is created exclusively (unlink, then
  `O_EXCL|O_NOFOLLOW`, 0600). On Windows `wxServer` is DDE
  (`wxUSE_DDE_FOR_IPC`): the service name is `ecce-<key>`, not a port; it is
  not private to the user, hence the token file the client must send.
  `OneInstance::release()` (from `MainFrame::shutdown`) closes the listener
  and removes lock, socket and token, so a start during a slow exit opens a
  window; files that arrive before the window exists are queued
  (`takePending`). Message: token, then absolute file names; the first copy
  does `MainFrame::bringToFront()` and `openFiles`. A copy that holds the lock
  but does not answer within 5 s is ignored and the new copy starts anyway.
- Files: `ecce FILE` (packaging/ecce.in) puts them in `ECCE_OPEN_FILES`,
  because the gateway sits between the launcher and the Organizer;
  `ecce.vbs` forwards its arguments to ecce.cmd, which passes `%*` to
  organizer.exe. `CalcMgr::openStructureFiles` starts a Builder with a
  `file://` URL. macOS: `CalcMgrApp::MacOpenFiles`.
- ecce-splash (Windows) ignores organizer.exe processes that were running
  before it, so a second start no longer says "window shown after 0 s", and
  treats an exit status 0 of the new one as the hand-over, not an error.
- Test: `apps_organizer_single_instance` (ECCE_SINGLE_ONLY=1 of
  one_process_test.py); `packaging_file_types` checks the package sources.
