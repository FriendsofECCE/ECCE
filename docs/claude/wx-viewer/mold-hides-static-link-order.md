---
type: pitfall
title: "mold links what GNU ld refuses: a missing static-library dependency passes locally and fails on every CI platform"
area: wx-viewer
paths: [CMakeLists.txt]
issues: [767]
---
The ECCE libraries are static. GNU ld (used by CI on every platform)
resolves symbols in link-line order, so a library that calls into another
must declare it (`target_link_libraries(<lib> PUBLIC <dependency>)`) or a
program whose `LINK` list names the dependency earlier fails with "undefined
reference". mold, which the developer and agent builds use
(`-DCMAKE_LINKER_TYPE=MOLD`), does not care about the order, so the build
passes locally. Happened 2026-11-01: eccedsi began to link eccercmd, whose
RCommand calls AuthCache (eccetdat), which calls CpuCores (ecceutil); 16
small programs failed on CI. After changing which library links which,
check with GNU ld before pushing: a separate build directory with
`-DCMAKE_LINKER_TYPE=BFD` (ccache makes the compile cheap) and `ninja` the
programs whose `LINK` lists are shortest.

On Windows the apps link their libraries as one `$<LINK_GROUP:RESCAN,...>`
group (ecce_app), so order inside the group does not matter, but a library
outside the group that depends back on a member is a cycle CMake refuses at
configure time ("strongly connected component"). So eccercmd's dependencies
on eccetdat/ecceutil are declared for non-Windows only, and on Windows
ecce_app adds eccercmd to the group whenever eccedsi is in it.
