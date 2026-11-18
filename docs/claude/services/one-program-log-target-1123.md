---
type: pitfall
title: "One program: the process has one wx log target; a modal wxLogGui box from a tool stops queued events on macOS (#1123)"
area: services
paths: ["src/tdat/resources/RefMachine.C", "src/comm/commtools/RunMgmt.C", "src/wxgui/jms/ToolLog.C", "include/wxgui/ToolLog.H", "src/apps/organizer/CalcMgrApp.C", "src/apps/builder/Builder.C", "src/wxgui/ewxClasses/ewxLogTextCtrl.C"]
issues: [1123, 1119, 1065, 1069, 983]
---
- `wxLog::SetActiveTarget` is per process. A separate Builder makes its log
  panel (`ewxLogTextCtrl`) the target; in one program the Organizer owns it.
  The default target, wxLogGui, shows a logged error or warning at the next
  idle as a modal box. On macOS that is `[NSAlert runModal]` from
  `wxAppConsoleBase::ProcessIdle`: while it is up no idle or `CallAfter`
  work runs, so hook commands answer "queued" and nothing happens, Quit
  included; the hook's `answer` cannot close it (not a wxDialog). Linux
  runs the queue inside the dialog's loop, so tests there pass.
- Fix: in one program `CalcMgrApp` installs `ToolLog` (a wxLogGui). A
  message goes to the log panel of the focused tool window or the active
  tab, if that tool registered one (`ToolLog::registerTool`; the Builder
  and Viewer do); anything else goes to one non-modal box, a child of the
  main window, so it closes with it and never holds the program.
- A tool that registers a panel unregisters it in its destructor. The
  `ewxLogTextCtrl` stays pushed on its text control (which may be gone
  first) and is `detach()`ed, not deleted: its flash timer must not touch
  the dead control.
- With `ECCE_TEST_ORGANIZER` the Organizer chains `wxLogStderr`, so log
  text reaches the test log; without it a macOS hang shows nothing.
- What the Builder logged on macOS was "The machine you are running on ...
  has just been added": uname gives "Name-MacBook-Pro.local", the resolver
  (and so MyMachines) "name-macbook-pro.local", and the case-sensitive
  `RefMachine::fullLookup`/`abbrevLookup` never matched, so every Organizer,
  Builder and Viewer start registered the machine again. Host names are
  compared without case now.
