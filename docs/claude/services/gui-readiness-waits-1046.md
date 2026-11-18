---
type: pitfall
title: "Details and Builder upgrade tests must wait for readiness (#1046)"
area: services
paths: [tests/apps/apps.py, tests/apps/calced_details_enabled_test.py, tests/apps/builder_upgrade_test.py]
issues: [1046]
---
The Details defaults arrive asynchronously from helper processes. A fixed
nine-second sleep can sample the Runtype Details button before its helper
sends values and `#INITIALIZED`. Use the existing CalcEd script `ready`
command before reading states and after each theory or run-type change.
It waits for both initialization flags, including helpers skipped because
their values are already stored; the harness still bounds the app lifetime.

Builder's `ECCE_PANEL_METRICS` file is written by a twelve-second timer
scheduled after property-panel creation. A window appearing does not mean
that timer has started or fired. `apps.run(..., ready=predicate)` treats
`settle` as a maximum wait and returns early when the predicate succeeds.
The upgrade test waits up to ninety seconds for its metrics file, retaining
the calculation-open and stored-version assertions.

The unchanged install-tree tests passed locally. Delaying the Python
Details helpers by twelve seconds reproduced `on/off` on open versus
`on/on` after re-choosing. Pausing Builder's process group for thirty-five
seconds at its first window reproduced "no panel metrics" with the old
thirty-second settle period. These controlled delays expose test timing
assumptions; they do not establish the exact delays on another host.
