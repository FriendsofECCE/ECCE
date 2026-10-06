---
type: checklist
title: "No `CMakeLists.txt install()` changes needed"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["data/", "scripts/*", "scripts/codereg"]
issues: [38]
---
**No `CMakeLists.txt install()` changes needed** for a new code's
own files — unlike [the other `scripts/*` gotcha](the-one-gotcha-that-has-bitten-this.md), `scripts/
parsers`, `scripts/codereg`, and `data/` are already installed as
whole directories (`install(DIRECTORY ...)`), so new files under
them are packaged automatically. (Confirmed still true adding MOPAC.)
