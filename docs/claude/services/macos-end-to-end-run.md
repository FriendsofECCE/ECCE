---
type: map
title: "tests/macos/e2e.sh: water single points on a Mac with real codes, through the test hooks"
area: services
paths: [tests/macos/e2e.sh, tests/macos/e2e-env.sh]
issues: [133]
---
Drives the installed ECCE.app (no clicks): Organizer (`ECCE_TEST_ORGANIZER`:
newproject, newcalc, state, summary), Builder (`ECCE_BUILDER_SCRIPT`: add O,
addh, save), Register Machines (`ECCE_MACHREG_SCRIPT`: login file, Find),
CalcEd (`ECCE_CALCED_SCRIPT`: theory, `basis NAME`, save), Launcher
(`ECCE_LAUNCHER_SCRIPT`: machine localhost, launch), then the job monitor
stores Props; the same Inputs are run directly and TE compared, and the
Builder is opened with `ECCE_OPEN_PANEL=MOs` + `ECCE_PANEL_METRICS`. Needs
`~/mopac`, `~/orca/...`, `~/miniforge3` (nwchem, wxpython). Run on the 10.15
VM (`ssh catalina`, VirtualBox on nerdegg): `E2E=$HOME/e2e bash e2e.sh
mopac nwchem ecceqm`. Screenshots are blank on that VM's display; Perl's
alarm stands in for timeout(1).
