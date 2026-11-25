---
type: pitfall
title: "A Windows drive colon in a file URL was parsed as host:port: every command-line request to the Organizer was refused (#1289)"
area: services
paths: ["src/util/genutil/EcceURL.C", "src/util/genutil/LocalData.C", "src/apps/organizer/CalcMgr.C", "src/wxgui/wxtools/WxResourceTreeCtrl.C", "tests/localdata/ecceurl_drive_test.C"]
issues: [1289, 1287]
---
- `EcceURL::parse` took the first ':' after "://" as a port separator even
  when it came after the authority's '/'. `file:///C:/Users/...` (what
  `LocalData::pathUrl`, so every ecce-reparse/ecce-mkdir/... sends on
  Windows) became host "/C", port 0, file "/Users/...", printed as
  `file:///C/Users/...`. `findNodeFresh` then matched nothing and the
  Organizer answered "not a calculation in this session" / "the parent is
  not a folder in this session" for any calculation.
- The tree's own URLs on Windows are `file://C:/...` (two slashes: the
  local server is `"file://" + LocalData::dir()`, children `"file://" + path`).
  `operator==`/`isChildOf` strip "file://" and compare text, so
  `file:///C:/x` and `file://C:/x` are different unless parsed to one form.
- Fix: a ':' is a port only inside the authority; on Windows a hostless
  file URL's "/C:/..." path is stored as "C:/...", the tree's form.
- The reply echoing `file:///C/...` (colon gone) is the tell. On Linux the
  same parse bug hit any local path with a ':' in it.
- Test: `ecceurl_drive` (Linux and the Windows CI ctest set).
