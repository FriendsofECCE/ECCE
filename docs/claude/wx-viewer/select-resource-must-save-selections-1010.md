---
type: pitfall
title: "ProjectContextPanel::selectResource set the cursor but left getSelections() stale (#1010)"
area: wx-viewer
paths: [src/apps/organizer/ProjectContextPanel.C, src/apps/organizer/CalcMgr.C, tests/apps/workflow_test.py]
issues: [1010]
---
`selectResource` only moves the grid cursor; `p_selections` (what
`getSelections()` and the tools read) is refreshed by the range-select event,
which can still be queued when the caller returns. After Make Workflow the
hook `workflowmake` read an empty selection ("unselected"), and a code button
clicked at once could see no step. `selectResource` now calls
`saveSelections()` itself, as `selectResources` already did.
