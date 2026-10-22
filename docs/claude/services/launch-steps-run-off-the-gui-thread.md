---
type: pitfall
title: The Launcher runs each Launch step on a worker thread; nothing a step calls may touch wx
area: services
paths: [src/apps/launcher/WxLauncher.C, src/comm/commxt/Launch.C, src/comm/rcommand/RCommand.C, src/wxgui/jms/WxJMSSubscriber.C, tests/queues/stubsched.py]
issues: [424]
---
`WxLauncher::runLaunchStep` runs `Launch::nextOperation` on a std::thread and
keeps the GUI thread in a `wxYield` loop, so the window answers the desktop
while ssh, the data server or the submit command wait. The calculation is
shared between the two threads, so while the loop runs:

- input is off (`wxWindowDisabler` over the whole launch, dialogs between
  steps included);
- broker messages are deferred, not dropped (`WxJMSSubscriber::deferMessages`):
  invoke, quit, state and machine-change callbacks run after the launch;
- the data-server login window is the one GUI call a step can make
  (`AuthEventListener::getAuthorization`); `WxLauncher::getAuthorization`
  sends it to the GUI thread with CallAfter and waits.

Anything new that a step calls must stay free of wx, or go through the GUI
thread the same way. Other `Launch` users (launchjob, MDPrep, JobParser) still
call `nextOperation` on their own thread.

The submit command's timeout (`ECCE_SUBMIT_TIMEOUT`, default 60 s) is an idle
limit: it counts from the last output, so a client that keeps printing retry
lines is not cut off. When it expires, `RCommand::timedOut()` is set and Launch
says "The queue (<manager>) did not answer within N s". Test:
`tests/queues/run_tests.py --suite stubs --manager pbs` (slow-submit rows,
stand-in qsub sleeping via `<spool>/pbs/slow_submit`; the Launcher row checks
the hook's `pumps` count).
