---
type: pitfall
title: "A workflow step may store no theory: DeckWriter and prepareStep must not dereference calc->theory() (#950)"
area: codereg
paths: [src/dsm/edsiimpl/DeckWriter.C, src/dsm/xml/JCode.C, src/apps/organizer/WorkflowSteps.C, src/apps/organizer/CalcMgr.C, tests/apps/workflow_test.py]
issues: [950, 910, 901, 926]
---
`ICalculation::theory()` returns a new `TTheory` or null when the `ecce:theory`
property is empty. Recipe steps made before #910 (2026-11-08) have none for
xTB and CREST: `applyRoleMethod` sets a theory only for a DFT/HF method, and
`applyDefaultTheory` did not exist yet. Since #901 `prepareStep` writes a
Created recipe step's deck, so opening such a workflow with auto-run on ran
`DeckWriter::generate` -> `writeGbs` -> `*(calc->theory())` on null and the
Organizer crashed (`JCode::theoryNeedsBasis` -> `TTheory::operator==`).

Now: `generate` refuses with "no theory yet", `writeGbs` checks it,
`theoryNeedsBasis` checks `p_theories`; `prepareStep` first stores the code's
default theory (`applyDefaultTheory`), and if there is still none the step
stays Created with `ecce:workflow.notready` "no theory yet". All 13 current
recipe/variant combinations store a theory on every step.

`CalcMgr::autoRunWorkflow` does nothing for a local workflow folder the user
cannot write to. The imported Examples are the user's own copies (writable);
their steps are all Complete, so auto-run has nothing to prepare there.

The CREST step New step makes after an xTB optimisation (inserted or the
source) is marked `ecce:workflow.auto` like that xTB step: it has nothing to
edit before xTB runs, and without the mark it stayed Created after xTB
completed, so auto-run stopped (#926 point 3; not a theory problem).
Test: `run_no_theory` and `run_hand_ready` in `apps_workflow_autorun`.
