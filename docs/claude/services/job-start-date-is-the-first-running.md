---
type: pitfall
title: "A calculation's start date is set by the first \"running\" only; a restarted monitor reports running again"
area: services
paths: ["src/comm/commxt/JobStore.C", "src/dsm/edsiimpl/TaskJob.C", "src/util/genutil/TDateTime.C", "include/util/TDateTime.H", "src/wxgui/wxtools/GridElement.C", "src/apps/organizer/CalculationContextPanel.C", "tests/launch/jobtimes_test.py"]
issues: [463]
---
`ecce:startdate` is written by `eccejobstore` in `calcUpdateState()` when
the monitor's status changes to 301 (running). `lastStatus` lives in the
store process, so every restarted store (monitor `fatal` -> `restart()`,
a killed monitor, a reconnect after silence) sees 301 as new and used to
write the start date again. ORCA `HCN_TS` (#470: six monitor restarts in
the last two seconds) ended with start = completion = 08:50:21 although
it ran from 08:35:17.

The start date is now written only while it is empty. A new run starts
empty because `TaskJob::setState()` clears both dates for any state up
to READY (launch from created/ready, reset for rerun/restart); catch-up
reconnect (`JobCatchUp::reconnect`, state -> submitted) keeps it, which
is right, since the same job is watched again.

Stored dates always had seconds (`TDateTime::toString()`, RFC 1123 GMT,
`%T`). Only the display dropped them: the Time Format preference
defaulted to `hh:mm`, and most users have it saved. Job grids
(`GridElement::getTimeFormatPref`) and the calculation context panel now
pass the preference through `TDateTime::withSeconds()`, so the `hh:mm`
choices show `hh:mm:ss`. `TDateTime(string)` also reads a value without
seconds. The job store runs on the client in every deployment mode, so
the same code writes the dates in local, central-server and shared-broker
setups; the data server only stores the property.
`tests/launch/jobtimes_test.py` (ctest `launch_jobtimes`) checks a
normal job and one whose monitor is killed and restarted.
