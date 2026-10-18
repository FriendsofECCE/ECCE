---
type: pitfall
title: A launch check reports a problem through ECCE-LAUNCH-WARNING on stdout, not through its exit status
area: codereg
paths: [scripts/parsers/gromacs.launchpp, src/comm/commxt/Launch.C, src/apps/launcher/WxLauncher.C]
issues: [930]
---
RCommand::execout does not return the exit status of a launchpp script beyond
success/failure, and a failure ends the launch. A check that should only warn
(the user decides) therefore exits 0 and prints `ECCE-LAUNCH-WARNING` and the
text; Launch::postProcessInput keeps the text (CR LF removed) and the Launcher
asks Cancel / Launch anyway. Exit 1 stays for real failures (missing files).
Headless: `launchwarn TEXT` in the Launcher script hook, and
LAUNCHJOB_WARNING=cancel for tests/launch/launchjob.
