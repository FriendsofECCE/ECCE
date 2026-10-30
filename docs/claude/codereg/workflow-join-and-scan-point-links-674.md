---
type: map
title: "Workflow joins and scan-point links (#674): link names carry the role and the point; the join stores the picks TS Search stores"
area: codereg
paths: [src/apps/organizer/WorkflowSteps.C, src/apps/organizer/WorkflowPanel.C, src/dsm/edsiimpl/ScanPoint.C, src/dsm/edsiimpl/QstStructures.C, tests/apps/workflow_test.py, data/client/examples/workflows]
issues: [674, 410, 419]
---
- Link name = `<role>` or `<role>@<point>`; role `geometry` (one source),
  `reactant`/`product`/`tsguess` (a join); point `last`, `max` or a 1-based
  number (`ScanPoint.H`). The arrow label is `WorkflowSteps::linkLabel`.
- A join is `WorkflowSteps::createJoin`: it sets the run type
  (`Transition State (QST) + Frequencies`, ORCA `(NEB-TS) + Frequencies`), the
  step's own structure (the reactant's) and the keys
  `ES.Runtype.GeomOpt.QST{Reactant,Product,Guess}` / `ES.Runtype.NEB.*` that
  CalcEd's `syncTsKeys` writes, so CalcEd opens it with the picks filled in.
  A pick is `<calc url>` or `<calc url>#point=<spec>`; `QstStructures`
  (`qstPickStructure`) is the one place that resolves it. It lives in
  libeccedsi (moved from src/apps/calced) so the Organizer can call it.
- `Node.in` lists every source; a step with several sources is placed one
  column right of the right-most source and gets a row of its own (it is a
  root of the row layout), so arrows to it pass below the boxes of the
  sources' column, never through them. Its box is taller: one slot per arrow,
  because each label sits above the last segment of its own arrow.
- Example `workflow.txt`: `<folder> < <source>[@point]` or, for a join,
  `<folder> < <source>=<role>[@point] ...`.
- Hooks: `workflowstep ... point=max`, `workflowjoin`, `workflowqst` (writes
  the .qstN.frag files into a temp dir and prints it).
