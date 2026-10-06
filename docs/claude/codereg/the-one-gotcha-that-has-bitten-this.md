---
type: map
title: "The one gotcha that has bitten this project repeatedly"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["scripts/*", "scripts/codereg"]
issues: []
---
**The one gotcha that has bitten this project repeatedly**: every file
under `scripts/*` needs its own `install()` rule in `CMakeLists.txt`,
or it's simply absent from the packaged `.deb` — correct in the repo,
"command not found" only when installed. Hit independently for
`std2NWChem`, `processmachine`, `gensub`, `eccejobmonitor`, and
`scripts/codereg`. If a script mysteriously isn't found only in the
packaged app, check `CMakeLists.txt` first.
