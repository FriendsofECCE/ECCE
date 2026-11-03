---
type: map
title: "Make Workflow moves a calculation into a Session; refused for workflow/study members and active jobs"
area: codereg
paths: [src/apps/organizer/CalcMgr.C, src/apps/launch/ecce-workflow.C, tests/apps/workflow_test.py]
issues: [807]
---
`CalcMgr::moveIntoWorkflow` creates the workflow with `createChild`, then
`Resource::move`s the calculation into it and adds the moved object (fetched
again by its new URL) with `addMember` + `saveLinkbase`. A calculation whose
job is submitted/running is refused (`activeJobIn`): results are written back
to the URL the job was launched with. Any calculation whose parent is a
Session is refused (a workflow, else a study). Hooks `workflowmake` and
`workflowmove`; the CLI `ecce-workflow` asks the running Organizer over
`ecce_workflow_request`/`_reply` and does nothing without one.
