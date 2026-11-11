---
type: pitfall
title: "An example workflow step that failed to import was lost silently and stayed lost (#954)"
area: codereg
paths: [src/apps/organizer/CalcMgr.C, data/client/examples/workflows, tests/apps/examples_workflow_steps_test.py]
issues: [954]
---
A user's Workflows/Aminopyridine_protonation held the xTB and both ORCA steps
but not the CREST step `ap_protonation`, and so no link at all: the links of
workflow.txt are made only when both ends are members (`joinExampleWorkflow`),
and the CREST step is the source of every one. The three steps were created in
one import a day earlier; on the current build a fresh import, an update from
an older set version and a top-up of exactly that broken copy all bring the
CREST step and its links back, so the original failure was not reproduced.
What made it silent and permanent:
- `importExamplesOnce` reported a failed set with a WARNING and replaced it at
  once with the INFO "now in Examples, sorted by topic". It now keeps the
  failures on the message line.
- `openExampleSet` reported only what the copy/parse callbacks set in
  `p_batchError`; a step with no calculation afterwards said nothing. It now
  checks every shipped calculation after the import and names the missing.
- The set is topped up only when the example-set version changes or the user
  opens it again, so a lost step stayed lost. At a start of the same version a
  workflow session missing one of its shipped steps is now topped up (workflow
  topics only: deleting other example calculations stays the user's choice).
Test: `tests/apps/examples_workflow_steps_test.py` (fresh, update, lost step
reopened, lost step at a plain start) asserts every step and link of every
workflow.txt.
