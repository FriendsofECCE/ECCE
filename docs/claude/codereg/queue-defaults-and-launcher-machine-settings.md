---
type: map
title: "Queue defaults come from the .Q file, the memory unit is MB, and the Launcher opens Register Machines on its machine"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/apps/machregister/WxMachineRegister.C", "src/apps/launcher/WxLauncher.C", "src/apps/launcher/WxLauncherScript.C", "scripts/processmachine", "siteconfig/submit.site", "tests/machregister/launcher_test.py"]
issues: ["234"]
---
**Defaults.** The Queues tab edits, per queue, `minProcessors`, `maxProcessors`,
`runLimit`, `memLimit`, `scratchLimit` and the four defaults the Launcher reads
(`WxLauncher::refreshProcessors/MemoryLimit/ScratchSpace/WallTime`):
`defProcessors`, `defRun` (minutes), `defMemory` (MB), `defScratch` (MB). The form
shows wall time in hours and memory/scratch in GB. A default of 0 is "none" and
writes no line. A limit of 0 means "no limit", so it bounds no default. The
Launcher clamps to the limits, so Add/Update Queue refuses a default outside
them (`queueDefaultsError`). `scratchLimit` is labelled "limit": the Launcher
uses it as the upper bound of its scratch field, not a minimum.

`processmachine` rewrites the five limit keys always, and a def key only when
the caller sent it (even empty); a caller that sends none (older scripts,
`config_test.py`) keeps hand-written `def*` lines. `memUnits` is never managed.

**The memory unit is MB, whatever `memUnits` says.** Nothing reads a queue's
`memUnits`: `Queue.C` parses and prints it. The Launcher puts MB into
`launch_maxmemory`, gensub's `-m` carries it as `$memory`, and `$memoryM` is
`$memory` followed by a literal `M`. `siteconfig/submit.site` said the unit came
from `memUnits`; the comment (also in the four golden job scripts under
`tests/queues/golden`) now says what decides it.

**Launcher.** The "Machine settings..." button next to the machine choice and
Tools > Register Machines both go through `machRegisterMenuitemClickCB`: an
`ecce_get_app` request to the gateway with `initmachine=<selected>`, so Register
Machines starts through the same path as from the Organizer. Machine Browser's
Machine > Register Machines does the same for its selected row. A save in
Register Machines publishes `ecce_machreg_changed`; the Launcher's handler is
`WxLauncher::machRegChanged` (mark preferences stale, drop `QueueManager`'s cache,
reload machines, queues and defaults).

`ECCE_LAUNCHER_SCRIPT=<file>` is the Launcher's test hook (header of
`WxLauncherScript.H`): no broker, and the button records the request it would
send. `tests/machregister/launcher_test.py` (ctest `machregister_launcher`)
checks the request, then runs a real Register Machines save and checks that the
Launcher shows the new queue and default. It does not cover the gateway start
itself.
