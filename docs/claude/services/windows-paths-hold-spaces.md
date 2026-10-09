---
type: pitfall
title: "Windows: the user's folder can hold a space, and every path ECCE builds sits below it"
area: services
paths: ["src/comm/rcommand/RCommand.C", "include/comm/RCommand.H", "src/comm/commxt/Launch.C", "src/comm/commxt/JobStore.C", "src/apps/calced/ESInputController.C", "src/apps/calced/CalcEd.C", "src/apps/calced/InputVerifier.C", "src/dsm/xml/JCode.C", "src/tdat/chemistry/SymmetryOps.C", "src/tdat/chemistry/Fragment.C", "scripts/gensub", "scripts/parsers/*.launchpp", "src/comm/rcommand/DirectTransportWin.C"]
issues: [247]
---
**ECCE_HOME (`%LOCALAPPDATA%\Programs\ECCE`), the temp folder, the
local data folder and the default run directory (`~/ecce-runs`) are all
below `C:\Users\<name>`, and a Windows user name can contain a space.**
Every command line that pastes one of these in unquoted breaks there,
usually silently: Save said it could not create the temporary
calculation directory (`test -d` on two words), the Theory/Runtype
dialogs got their temp file as two arguments, gensub read the run
directory up to the space from its param file, and a launchpp split
`runDir: C:/...` at the drive colon.

Rules: RCommand's file operations (`exists`, `directory`, `writable`,
`cd`, `shellget`) take a *path*, not shell syntax; they quote it
themselves. Command lines built elsewhere quote with
`RCommand::quotePath()` (single quotes, a leading `~`/`~/` left bare so a
remote shell still expands it) or `quoteGlob()` for a pattern; a line
that may reach cmd.exe (`Ecce::runCommand`, `CreateProcess`) uses double
quotes. gensub writes the run directory through `shWord()`, which leaves a
plain path as it is, so the golden scripts did not change. A `key: value`
reader splits at the first colon or space only.

Test it with a tree *and* a state folder under a path with a space:
`tests/windows/e2e_win.py "<tree with space>" "<state with space>"` (its
temp folder is below the state folder); `tests/transport/testRCommandDirect.C`
(`spaceChecks`) and `tests/queues/run_tests.py` (`spacedRunDir`) cover the
Linux side. An `msiexec /a` tree has no `tmp` folder (the installer creates
it), and MSYS then prints "could not find /tmp" on every shell start; make
one when testing from such a tree.
