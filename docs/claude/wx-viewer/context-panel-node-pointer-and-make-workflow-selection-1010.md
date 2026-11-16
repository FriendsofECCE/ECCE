---
type: pitfall
title: "The Organizer's context panel is matched to its tree node by address; a deleted node left it dangling, and Make Workflow could select the step in a panel that was not the workflow's (#1010)"
area: wx-viewer
paths: [src/apps/organizer/CalcMgr.C, src/wxgui/wxtools/WxResourceTreeCtrl.C, tests/apps/workflow_test.py]
issues: [1010, 807, 951]
---
`CalcMgr::setContextPanel()` decides between "refresh the panel shown" and
"build a new one" with `newNode == p_panelNode`, a pointer compare. Tree
items are deleted and remade by `WxResourceTreeCtrl::refresh` /
`loadChildren(node, true)` (every child of the refreshed folder), so
`p_panelNode` could point at freed item data. When the allocator handed the
same address to the next item made, a newly selected node was taken for the
old one: the old panel was refreshed and the new node's panel never built.
Which address comes back depends on the heap and on the order the folder's
children are read, so it differs between hosts. `OnTreectrlDeleteItem` now
clears `p_panelNode` when its item is deleted.

Make Workflow (`moveIntoWorkflow`) deletes the moved calculation's tree item
with `refresh(folder)`, then `showInWorkflow` selects the workflow with
`findNode(wf, true)` and the step with `selectResource`. `SelectItem` on the
item that is already the tree's selection sends no event, so the panel is
not switched either. `showInWorkflow` now builds the workflow's panel itself
when the panel's node is not the workflow's, before selecting the step.

On the wx generic tree, deleting the selected item clears the selection;
the replacement is selected later, from idle (`m_select_me` in
`OnInternalIdle`), which `wxYield` runs.

The `workflowmake` hook answers `view=workflow selected` only when the panel
shown is the workflow's (`SessionProjectContextPanel` with a `WorkflowPanel`,
panel node = the workflow) and its table selection holds the step; the
boxes follow the table (#951). The answer `error: no such calculation` in
the same check's output is the expected one for the second command
(`workflowmake CALC again`), which the test's prefix match also picks up.
