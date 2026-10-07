---
type: rule
title: "Full builds and long suites go to radium/tellurium via tools/offload"
area: services
section: "Rules"
paths: ["tools/offload/run-offload.sh", "tools/offload/remote-run.sh", "tools/offload/Containerfile"]
issues: []
---
**Do not run full builds, `tests/teaching` (about 26 min on niobium) or other
long suites on niobium; use `tools/offload/run-offload.sh <pushed ref>
<build|ctest|teaching|apps|run>`.** The ref must be pushed first; the host
fetches from GitHub. It prints a summary and a PASS/FAIL line, and keeps the
full log on the host. radium (container, 2 slots) and tellurium (native, 3
slots, work root `/mnt/games/ecce` only, off after 20:30) are picked
automatically; exit status 3 means none is reachable, so run locally. Inside
radium's container `$USER` must be set or the test data server creates no
account. See `tools/offload/README.md`.
