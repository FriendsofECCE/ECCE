---
type: pitfall
title: "Workflow autorun tests wait for command replies, not new X11 window IDs (#1081)"
area: codereg
paths: [tests/apps/workflow_test.py, tests/apps/apps.py, src/apps/organizer/CalcMgr.C]
issues: [1081]
---
`apps_workflow_autorun` restarts the Organizer ten times. The old fixed
settle periods alone totalled 480 seconds against a 600-second ctest limit.
On NerdEgg, a restart could snapshot the previous Organizer's windows before
X11 removed them. The next Organizer reused their IDs (`0x20010c`,
`0x200003`, `0x200001` in the diagnostic run). `apps.run` compares IDs with
that snapshot, so it reported `sawWindow=False` and spent its whole
60-second window timeout, then the full settle period. A measured session
took 120.75 seconds despite answering every command. The unchanged full
test passed when allowed to continue beyond 600 seconds. Buffered Python
output hid its progress from ctest.

The autorun cases now use `run_script`: the existing
`ECCE_TEST_ORGANIZER_ANSWERS` hook acknowledges every command, including
explicit `idle` commands that allow background workflow updates. It bypasses
window discovery and fixed settling, checks complete response lines, and
stops after all replies. A missing reply fails after 60 seconds with the
unanswered command and the app log tail; `ScriptTimeout` ends the test
through its normal service and data cleanup. Progress and failures flush
immediately. Other app tests still use the shared window-discovery harness.
