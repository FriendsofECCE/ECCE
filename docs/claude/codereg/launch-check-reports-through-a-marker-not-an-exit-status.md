---
type: pitfall
title: A launch check reports a problem through ECCE-LAUNCH-WARNING on stdout, not through its exit status
area: codereg
paths: [scripts/parsers/gromacs.launchpp, scripts/parsers/GromppWarn.pm, src/comm/commxt/Launch.C, src/apps/launcher/WxLauncher.C, src/wxgui/wxdialogs/ewxMessageDialog.C]
issues: [930]
---
RCommand::execout does not return the exit status of a launchpp script beyond
success/failure, and a failure ends the launch. A check that should only warn
(the user decides) therefore exits 0 and prints `ECCE-LAUNCH-WARNING` and the
text; Launch::postProcessInput keeps the text (CR LF removed) and the Launcher
asks Cancel / Launch anyway. Exit 1 stays for real failures (missing files).
Headless: `launchwarn TEXT` in the Launcher script hook, and
LAUNCHJOB_WARNING=cancel for tests/launch/launchjob.

The text after the marker is the heading "The job will stop at grompp with this
error:" (or pdb2gmx), a blank line and the body. For the common stopping
grompp warnings the body is ECCE's own wording (`GromppWarn::explain`: net
charge with its value, tau-t/tau-p too short, constraints on all bonds, plain
Coulomb cut-off, COM removal with position restraints); anything else is
grompp's text. When ECCE reworded, grompp's message follows a line
`ECCE-ORIGINAL-MESSAGE` and the Launcher shows it in a closed pane
(`ewxMessageDialog::AddDetails`). The text is UTF-8 (a real minus sign), so
read it with `FromUTF8`, never `std::string` conversion. Hook:
`launchwarn 'head\n\nbody\nECCE-ORIGINAL-MESSAGE\noriginal'` (literal `\n`).
