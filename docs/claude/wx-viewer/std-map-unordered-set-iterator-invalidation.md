---
type: map
title: "`std::map`/`unordered_set` iterator invalidation"
area: wx-viewer
section: ""
paths: ["AuthCache.C", "DavEDSI.C", "GUIValues.C"]
---
**`std::map`/`unordered_set` iterator invalidation**: `erase(it)`
followed by a loop's own `it++`/`--it` touches a dangling iterator.
Was found in 17 places codebase-wide in one audit
(`GUIValues.C`, `AuthCache.C`, `DavEDSI.C`, others) — grep
`erase(it)`/`erase(iter)` if you're touching map/set cleanup code.
Correct pattern: `it = container.erase(it);`.
