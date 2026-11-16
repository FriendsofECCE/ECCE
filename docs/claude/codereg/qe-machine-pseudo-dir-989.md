---
type: map
title: "A machine's QE pseudopotential directory reaches the deck through postParams, and its check uses Launch's own login"
area: codereg
paths: ["src/comm/commxt/Launch.C", "scripts/parsers/qe.launchpp", "src/apps/machregister/WxMachineRegister.C", "tests/launch/qe_pseudodir_test.py"]
issues: [989]
---
`QuantumESPRESSOPseudoDir` in CONFIG.<machine> (Register Machines, Codes tab,
shown for Quantum ESPRESSO only) is a machine-file key like `<Code>Modules`.
`Launch::postProcessInput` is the one step every launch path runs (Launcher,
`launchjob`), so it reads the key (`machinePseudoDir`,
`<codeName>PseudoDir`) and passes it to qe.launchpp as `pseudoDir:` in
postParams; the script replaces the `pseudo_dir` line only when it is given,
so a machine without the key gets the deck ai.qe wrote. ai.qe still picks the
UPF names from the local library.

`Launch::checkPseudoFiles` then runs one sh command over `p_connection`, the
login `validateRemoteLogin` already opened (no new credentials), with the
directory and file names quoted by `RCommand::quotePath`. Missing files or a
missing directory become a launch warning (Cancel / Launch anyway); no
connection (a no-remote-access machine) or an unrecognised answer sets the
info line "Could not check ..." and no warning. Skipped on reparse/import.
