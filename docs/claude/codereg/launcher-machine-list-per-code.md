---
type: pitfall
title: "The Launcher lists a machine for a code when its Machines line lists the code or CONFIG gives it a path"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/apps/launcher/WxLauncher.C", "src/dsm/xml/MachinePreferences.C", "src/tdat/resources/RefMachine.C", "include/tdat/RefMachine.H", "tests/launch/machines_test.py", "tests/launch/launchjob.C", "siteconfig/Machines"]
issues: []
---
`MachinePreferences::itemsForCode(code)` is the Launcher's machine list:
every registered machine (MyMachines, then site Machines) whose
`RefMachine::offersCode(code)` holds, i.e. the code is on its Machines line
**or** `CONFIG.<refname>` (site or user, merged) has a non-empty path for it.
The Launch button checks the same predicate.

Why both: a MyMachines line **shadows** the site line of the same name
entirely, codes included, and Register Machines writes the codes that had a
path at save time. A user who saved `localhost` while their
`CONFIG.localhost` named only NWChem and Gaussian-16, then added `ORCA:` by
hand, got a Launcher without localhost for ORCA, while the site line listed
ORCA. Before 9.0.0-alpha.5 the list used the Machines line alone (8.18.5 and
alpha.4 alike; the CONFIG merge and `ConfigFile` changes did not touch it).

`launchjob machines <code>` prints the list headlessly;
`tests/launch/machines_test.py` (ctest `launch_machines`) checks it in a test
home with site and user files.
