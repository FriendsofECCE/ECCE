---
type: pitfall
title: "A GROMACS calculation from files is its own type (gromacs_md_run, ecceMdRun); a type sharing a content type with another cannot be told apart on reload"
area: codereg
paths: [data/client/config/ResourceDescriptorRxn.xml, data/client/config/ResourceDescriptor.xml, src/dsm/xml/ResourceDescriptor.C, include/dsm/ResourceDescriptor.H, src/util/genutil/ResourceUtils.C, src/dsm/edsiimpl/GromacsCalc.C, src/dsm/edsiimpl/Series.C, src/apps/organizer/GromacsImport.C, src/apps/organizer/CalcMgr.C, src/comm/commxt/Launch.C, scripts/gensub, scripts/parsers/gromacs.launchpp, data/client/cap/GROMACS.edml, src/dsm/edsiimpl/MdTask.C]
issues: [1038]
---
A stored resource is mapped back to its type by (resource type, content type,
application type), first match in the descriptor file
(`ResourceDescriptor::getResourceType(TNode*)`). A second type sharing
`ecceMdDynamics`/`GROMACS` with `gromacs_md_dynamics` would reload as
whichever comes first, so the calculation from files has its own content type
`ecceMdRun` (`CT_MDRUN`, mapped in `ResourceUtils`) and resource type
`gromacs_md_run`: no MDDynamics tool (that editor rewrites the `.mdp`),
contained by `project`, offered under File > New. A new `CT_` value must also
be added wherever the MD types are listed (`TaskJob` serializer,
`ChemistryTask::isFragmentReadOnly`, the Builder's `CalculationFactory`).

- `EDSIFactory::createTask` makes an MD task only inside a session;
  `CT_MDRUN` is let through without one.
- `MdTask::copy` refuses a task outside a study; `CT_MDRUN` copies as a
  plain resource (Duplicate, Continue from Last Geometry, Copy).
- `MdTask`'s follower walks (`resetForRerun`, `resetForRestart`,
  `canChangeState`, `getDeleteMessage`) go through `followers()`, empty
  with no session; `getSession()->` on a `CT_MDRUN` task is a SIGSEGV.
- Launch: the restart/topology provider search is inside `if (session != 0)`;
  a `CT_MDRUN` task takes its own attached start file and topology.
- A ready `.tpr` is stored as `run.gmxtpr` (its own type,
  `chemical/x-gromacs-run-tpr`; `.tpr` is the task's output type and Launch
  ships the output list, not inputs). gensub copies it to `topol.tpr` and
  skips grompp; gromacs.launchpp skips its grompp check.
- It is an MdTask, not an ICalculation: the hand-offs that assume
  ICalculation (Series' `loadStructure`, In Another Code's source filter,
  the Organizer's `isCalculationNode`) test `GromacsCalc::isRun` as well.
- Structures in and out (`GromacsCalc`): out is the latest `.gro` under
  Outputs (confout.gro); in replaces the coordinates of the latest input
  `.gro`, refused unless the atoms are the same in the same order (a `.gro`
  atom name with no known element, "X", is trusted by position).
- Continue from Last Geometry must take the final frame before
  `resetForRerun`, which removes the copy's outputs.
