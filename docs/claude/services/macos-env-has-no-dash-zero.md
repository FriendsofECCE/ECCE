---
type: pitfall
title: "macOS: BSD env has no -0, so a machine's login file was silently ignored (#133)"
area: services
paths: [src/comm/rcommand/RCommand.C, src/apps/machregister/SchedulerQuery.C]
issues: [133]
---
`RCommand::importSourceFile` finds what the machine's `sourceFile` ("Script
run at login") sets by diffing `env -0` before and after sourcing it. BSD
`env` rejects `-0`, both dumps came back empty, "0 variable(s) imported" was
a success, and Register Machines' **Find** then reported "No mopac was
found" for programs the login file had put on the PATH. The dump is now
`env -0 || perl -e 'print map {qq($_=$ENV{$_}\0)} keys %ENV'`. Same family:
macOS has no `readlink -f` (Find's script falls back to the link itself) and
no `timeout(1)`.
