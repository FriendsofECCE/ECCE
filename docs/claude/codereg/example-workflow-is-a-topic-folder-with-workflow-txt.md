---
type: map
title: "An example workflow is a topic folder with a workflow.txt; its steps join the session when each import is done"
area: codereg
paths: [data/client/examples/workflows, src/apps/organizer/CalcMgr.C, src/dsm/edsiimpl/StoredCalcCopy.C, data/client/config/ResourceDescriptor.xml]
issues: [341]
---
`data/client/examples/<set>/<topic>/workflow.txt` makes the topic a Workflow
session instead of a project. One line per example folder: `<folder>` for a
first step, `<folder> < <source folder>` for a step made from another; the
link is the stage-1 `geometry` link. Folders are ordinary examples
(output plus `stored/`), so `preparse.py` and the preparsed test need nothing
special.

- Steps are imported INTO the session: `getContainer` stops at a `workflow`
  node while `p_batchImport` is set, and `import_es` is in the workflow's
  `Contains` (both ResourceDescriptor files), otherwise `createChild`
  fails with "Can't access or write to the specified parent project".
- A step becomes a member, and links are made, in `joinExampleWorkflow`: from
  `importValidationComplete` (parse; it runs before the batch early return) or
  right after `StoredCalcCopy::copy`. Importing first into a project and
  copying over does not work, the import is not finished at that point.
- `annotate` must recurse into `workflow` nodes or the notes are missing and
  `examples_arm.py` never sees the set as stable.
- The session's `taskIndex` is the order in the importing session and is not
  copied (`StoredCalcCopy` skips it; `VOLATILE` in the preparsed test too).
- Test: `tests/apps/examples_workflow_test.py` (`apps_examples_workflow`).

A line `<folder> < <source folder>+hess` also makes the step read the
source's Hessian: `joinExampleWorkflow` stores the `<calc>.hess` that sits in
the source's example folder as the step's `inhess.hess`
(`OrbitalGuess::attachHessianFile`) and sets `ecce:hessianfromsource`;
`ESInputController` then writes Initial Hessian = Read from previous
calculation unless the dialog chose another value. The parse of the source
stores no `.hess`, so the file comes from the folder; the step's stored copy
carries it in `Inputs/` and the copy route needs no attach.
