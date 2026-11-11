---
type: pitfall
title: "Auto-run is armed by the tick and started by Launch Ready Steps; no editor opens by itself in a workflow (#926)"
area: codereg
paths: [src/apps/organizer/CalcMgr.C, src/apps/organizer/WorkflowPanel.C, src/apps/organizer/WorkflowSteps.C, src/apps/organizer/RecipePanel.C, help/src/workflows.md, tests/apps/workflow_test.py]
issues: [926, 736, 950, 902]
---
Two workflow properties: `ecce:autorun` (the checkbox) and
`ecce:autorun.started` (set by `CalcMgr::launchReadySteps`, i.e. the
Launch Ready Steps button and the `workflowlaunch` hook). `autoRunWorkflow`
launches only when both are "1". Ticking the box and opening a workflow call
nothing that launches; the WorkflowPanel constructor's catch-up call is gone
(it was also the path of the #950 crash). Unticking (`setAutoRun(false)`)
clears `started`; so does `autoRunWorkflow` once every step is Complete.
Steps that completed while no Organizer ran are not caught up: the user
presses Launch Ready Steps. The play button on a box (`launchStep`) does not
start a run.

`newWorkflowStep` and `newWorkflowJoin` no longer call `startApp` for the
Calculation Editor; `ask` only controls the New step / join dialog. Recipes
and Make Workflow never opened one. The `workflownew` hook used to skip the
editor through `p_testStepDialogs`, so a test could not see it open.
Tests: `run_autorun` (three Organizer runs) and the `workflownew` check in
`run_hand_ready`, `apps_workflow_autorun`.
